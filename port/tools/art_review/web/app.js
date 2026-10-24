const $ = id => document.getElementById(id);
let assets = [], approvals = [], current = null, mode = 'draw', start = null, draft = null, editing = null, selected = null;
let historical=false, chosenVerdict='pending';
let followLatest=true, loadedBuild=null, pendingUpdate=null, checkingUpdate=false;
let libraryTab='pending';
const deskTop=document.querySelector('.workspace-top'),deskToolbar=document.querySelector('.toolbar');
function positionDeskControls(){
  const inSidebar=window.innerWidth>1000&&!document.body.classList.contains('focus-mode');
  if(inSidebar){$('deskControls').append(deskTop,deskToolbar);document.body.classList.add('side-controls');}
  else{const w=document.querySelector('.workspace');w.insertBefore(deskTop,$('approvalInfo'));w.insertBefore(deskToolbar,$('updates'));document.body.classList.remove('side-controls');}
  if(window.innerWidth<=760||document.body.classList.contains('focus-mode'))document.querySelector('.workspace').insertBefore($('libraryTabs'),deskTop);
  else document.querySelector('.library').insertBefore($('libraryTabs'),$('searchPieces'));
}
positionDeskControls();
let scale = 1, saveChain = Promise.resolve(), changed = false, generalTimer, lastPath = '', fitMode = true;
const labels = {pending:'Pendiente de revisión', changes_requested:'Necesita mejoras', approved:'Aprobado', honors:'★ Aprobado con honores'};
const categories = {general:'General', silhouette:'Silueta / anatomía', seam:'Corte / unión', tiles:'Tiles', palette:'Color / luz', perspective:'Perspectiva', provenance:'Procedencia', app:'Aplicación'};
function toast(message) { $('toast').textContent = message; $('toast').hidden = false; setTimeout(() => $('toast').hidden = true, 6500); }
async function api(path, data) {
  const response = await fetch(path, data === undefined ? {} : {method:'POST', headers:{'Content-Type':'application/json','X-Art-Review':'local'},body:JSON.stringify(data)});
  const result = await response.json(); if (!response.ok) throw Error(result.error || 'No se pudo guardar'); return result;
}
function el(tag, cls, text) { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }
const revisionNav=el('div','revision-nav'),revisionLabel=el('label','','Revisión'),revisionPicker=el('select'),previousRevision=el('button','','←'),nextRevision=el('button','','→');
revisionPicker.id='revisionPicker';revisionPicker.setAttribute('aria-label','Revisión de la pieza');previousRevision.setAttribute('aria-label','Revisión anterior');nextRevision.setAttribute('aria-label','Revisión siguiente');revisionLabel.append(revisionPicker);revisionNav.append(previousRevision,revisionLabel,nextRevision);$('title').after(revisionNav);
function pieceVersions(){const family=approvals.find(f=>f.version_ids.includes(current?.id));return family?family.version_ids.map(id=>assets.find(a=>a.id===id)).filter(Boolean):current?[current]:[];}
function showRevisions(){
  const versions=pieceVersions();revisionPicker.replaceChildren();
  for(const [i,a] of versions.entries()){const option=el('option','',`R${i+1} · ${a.version} · ${a.annotations.length} nube(s)`);option.value=a.id;option.selected=a.id===current.id;revisionPicker.append(option);}
  const index=versions.findIndex(a=>a.id===current.id);previousRevision.disabled=index<=0;nextRevision.disabled=index>=versions.length-1;
}
revisionPicker.onchange=()=>choose(revisionPicker.value);
previousRevision.onclick=()=>{const list=pieceVersions(),i=list.findIndex(a=>a.id===current.id);if(i>0)choose(list[i-1].id);};
nextRevision.onclick=()=>{const list=pieceVersions(),i=list.findIndex(a=>a.id===current.id);if(i<list.length-1)choose(list[i+1].id);};
function gallery() {
  $('gallery').replaceChildren();
  $('assetPicker').replaceChildren();
  const byId=new Map(assets.map(a=>[a.id,a])),groups=new Map();
  for(const a of assets){let root=a;const seen=new Set();while(root.parent_id&&byId.has(root.parent_id)&&!seen.has(root.id)){seen.add(root.id);root=byId.get(root.parent_id);}if(!groups.has(root.id))groups.set(root.id,{root,versions:[]});groups.get(root.id).versions.push(a);}
  const search=$('searchPieces').value.trim().toLocaleLowerCase('es');let total=0;
  for (const {root,versions} of groups.values()) {
    const last=versions.at(-1),family=approvals.find(f=>f.family_id===root.id);
    const a=libraryTab==='accepted'?byId.get(family?.approved_version):(['pending','changes_requested'].includes(last.status)?last:null);
    if(!a||search&&!(`${root.title} ${a.version}`).toLocaleLowerCase('es').includes(search))continue;total++;
    const option=el('option','',root.title+' · '+a.version);option.value=a.id;option.selected=a.id===current?.id;$('assetPicker').append(option);
    const button = el('button','asset' + (versions.some(v=>v.id===current?.id) ? ' active' : ''));
    const img = el('img'); img.src = a.image_url;img.loading='lazy';img.alt = ''; button.append(img,el('strong','',root.title),el('small',a.status==='honors'?'honors':'',labels[a.status]),el('small','',a.version),el('small','',versions.length+' revisión(es) · ver Historial'));
    button.onclick = () => choose(a.id,{follow:libraryTab==='pending'}); $('gallery').append(button);
  }
  if(current&&!Array.from($('assetPicker').options).some(o=>o.value===current.id)){
    const pinned=el('option','',`En revisión: ${current.title} · ${labels[current.status]} (fuera del filtro)`);pinned.value=current.id;pinned.selected=true;$('assetPicker').prepend(pinned);
  }
  if(!total)$('gallery').append(el('p','subtle','No hay piezas en esta pestaña.'));
  $('tabPending').setAttribute('aria-selected',String(libraryTab==='pending'));$('tabAccepted').setAttribute('aria-selected',String(libraryTab==='accepted'));
}
function setLibraryTab(value){libraryTab=value;gallery();}
$('tabPending').onclick=()=>setLibraryTab('pending');$('tabAccepted').onclick=()=>setLibraryTab('accepted');$('searchPieces').oninput=gallery;
async function choose(id, options={}) {
  try { await flushGeneral(); await saveChain; } catch(e) { return toast(e.message); }
  current = assets.find(a => a.id === id); selected = null;
  libraryTab=['approved','honors'].includes(current.status)?'accepted':'pending';
  followLatest=options.follow===true;
  const address=new URL(location.href);address.searchParams.set('image',id);address.searchParams.delete('latest');history.replaceState(null,'',address);
  if(followLatest)address.searchParams.delete('view');else address.searchParams.set('view','history');history.replaceState(null,'',address);
  historical=false;$('historyBanner').hidden=true;
  for(const k of ['status','general','draw','submit','rate'])$(k).disabled=false;
  localStorage.setItem('atlas-selected', id);
  $('title').textContent = current.title; $('version').textContent = current.version;
  $('imageSize').textContent = `${current.width} × ${current.height} px`;
  $('provenance').textContent = current.provenance;
  $('revisionInfo').textContent = Object.keys(current.revision_info||{}).length?JSON.stringify(current.revision_info,null,2):'Primera entrega: aún no hay cambios vinculados a nubes anteriores.';
  $('status').value = current.status; $('general').value = current.general;
  $('empty').hidden = true; $('paper').hidden = false;
  $('art').src = current.image_url; $('art').alt = current.title;
  $('overlay').setAttribute('viewBox',`0 0 ${current.width} ${current.height}`);
  gallery(); renderNotes(); fit(); showApproval(); showRevisions(); $('saved').textContent = 'Guardado en este PC';
}
function showApproval(){
  const family=approvals.find(f=>f.version_ids.includes(current?.id));
  const approved=assets.find(a=>a.id===family?.approved_version),build=assets.find(a=>a.id===family?.build_version);
  $('approvalInfo').textContent=build?'★ Build conserva: '+build.version:approved?'✓ Aprobación conservada: '+approved.version+' · integración según política de honores':'';
  $('rate').textContent=historical?'Histórico':labels[current.status]+' ▾';
}
async function updateRegistry(){const state=await api('/api/state');approvals=state.approvals||[];showApproval();}
function resize() {
  if (!current) return;
  $('paper').style.width = current.width * scale + 'px'; $('paper').style.height = current.height * scale + 'px';
  $('zoom').value = scale * 100; $('zoomValue').textContent = Math.round(scale * 100) + '%';$('zoomNumber').value=Math.round(scale*100); renderClouds();
}
function fit() { if (!current) return; fitMode=true; scale = Math.max(.03,Math.min(($('viewport').clientWidth - 24) / current.width, ($('viewport').clientHeight - 24) / current.height, 4)); resize(); $('viewport').scrollTo(0,0); }
function zoomTo(value, anchor) {
  if(!current)return; fitMode=false;
  const viewport=$('viewport'),before=$('paper').getBoundingClientRect(),vr=viewport.getBoundingClientRect();
  const p=anchor||{x:vr.left+viewport.clientWidth/2,y:vr.top+viewport.clientHeight/2};
  const px=(p.x-before.left)/scale,py=(p.y-before.top)/scale;
  scale=Math.max(.05,Math.min(32,value));resize();
  const after=$('paper').getBoundingClientRect();viewport.scrollLeft+=after.left+px*scale-p.x;viewport.scrollTop+=after.top+py*scale-p.y;
}
$('fit').onclick = fit; $('zoom').oninput = () => zoomTo(Number($('zoom').value)/100);
$('zoom').max='3200';
const zoomNumber=el('input');zoomNumber.id='zoomNumber';zoomNumber.type='number';zoomNumber.min='5';zoomNumber.max='3200';zoomNumber.setAttribute('aria-label','Zoom en porcentaje');$('zoomValue').before(zoomNumber);$('zoomValue').hidden=true;zoomNumber.onchange=()=>zoomTo(Number(zoomNumber.value)/100);
const fitWidth=el('button','','Ancho');fitWidth.id='fitWidth';$('fit').after(fitWidth);fitWidth.onclick=()=>zoomTo(($('viewport').clientWidth-24)/current.width);
$('actual').onclick=()=>zoomTo(1);$('minus').onclick=()=>zoomTo(scale/1.5);$('plus').onclick=()=>zoomTo(scale*1.5);
$('viewport').addEventListener('wheel',e=>{if(!e.ctrlKey&&!e.metaKey)return;e.preventDefault();zoomTo(scale*Math.exp(-e.deltaY*.004),{x:e.clientX,y:e.clientY});},{passive:false});
window.addEventListener('resize', ()=>{positionDeskControls();if(fitMode)fit();else resize();});
$('focus').onclick=()=>{document.body.classList.toggle('focus-mode');positionDeskControls();$('focus').textContent=document.body.classList.contains('focus-mode')?'Restaurar paneles ⛶':'Ampliar mesa ⛶';fit();};
let spaceHeld=false,pan=null;
window.addEventListener('keydown',e=>{if(e.code==='Space'&&!document.querySelector('dialog[open]')&&!['INPUT','TEXTAREA','SELECT','BUTTON'].includes(document.activeElement?.tagName)){spaceHeld=true;e.preventDefault();}});
window.addEventListener('keyup',e=>{if(e.code==='Space')spaceHeld=false;});window.addEventListener('blur',()=>{spaceHeld=false;pan=null;});
$('viewport').addEventListener('pointerdown',e=>{if(e.button===1||(spaceHeld&&e.button===0)){e.preventDefault();e.stopPropagation();pan={x:e.clientX,y:e.clientY,left:$('viewport').scrollLeft,top:$('viewport').scrollTop};$('viewport').setPointerCapture(e.pointerId);}},true);
$('viewport').addEventListener('pointermove',e=>{if(pan){$('viewport').scrollLeft=pan.left+pan.x-e.clientX;$('viewport').scrollTop=pan.top+pan.y-e.clientY;}});
$('viewport').addEventListener('pointerup',()=>pan=null);$('viewport').addEventListener('pointercancel',()=>pan=null);
$('showNotes').onclick=()=>document.body.classList.toggle('notes-open');$('closeNotes').onclick=()=>document.body.classList.remove('notes-open');
$('assetPicker').onchange=()=>choose($('assetPicker').value);
$('refresh').onclick=async()=>{try{await flushGeneral();await saveChain;const state=await api('/api/state');assets=state.assets;approvals=state.approvals||[];await choose(current?.id||assets[0].id);toast('Versiones actualizadas');}catch(e){toast(e.message);}};
$('density').value=localStorage.getItem('atlas-density')||'compact';document.body.dataset.density=$('density').value;
$('density').onchange=()=>{document.body.dataset.density=$('density').value;localStorage.setItem('atlas-density',$('density').value);if(fitMode)fit();};
function setMode(value) { mode=value; $('draw').classList.toggle('active',value==='draw'); $('select').classList.toggle('active',value==='select'); $('overlay').classList.toggle('selecting',value==='select'); }
$('draw').onclick = () => setMode('draw'); $('select').onclick = () => setMode('select');
function point(e) { const r = $('overlay').getBoundingClientRect(); return {x:Math.max(0,Math.min(1,(e.clientX-r.left)/r.width)),y:Math.max(0,Math.min(1,(e.clientY-r.top)/r.height))}; }
function rectangle(a,b) { return {x:Math.min(a.x,b.x),y:Math.min(a.y,b.y),w:Math.abs(a.x-b.x),h:Math.abs(a.y-b.y)}; }
function pixels(n){return [Math.floor(n.x*current.width),Math.floor(n.y*current.height),Math.min(current.width,Math.ceil((n.x+n.w)*current.width)),Math.min(current.height,Math.ceil((n.y+n.h)*current.height))];}
function svg(tag, attrs) { const e=document.createElementNS('http://www.w3.org/2000/svg',tag); for(const [k,v] of Object.entries(attrs)) e.setAttribute(k,v); return e; }
function cloudPath(n) {
  const x=n.x*current.width,y=n.y*current.height,w=n.w*current.width,h=n.h*current.height;
  let d=`M ${x} ${y}`;
  const step=19/scale, bump=5/scale;
  for(const [ax,ay,bx,by] of [[x,y,x+w,y],[x+w,y,x+w,y+h],[x+w,y+h,x,y+h],[x,y+h,x,y]]) {
    const len=Math.hypot(bx-ax,by-ay),count=Math.max(1,Math.ceil(len/step));
    for(let i=0;i<count;i++) {
      const t=(i+.5)/count,u=(i+1)/count, nx=(by-ay)/len,ny=-(bx-ax)/len;
      d+=` Q ${ax+(bx-ax)*t+nx*bump} ${ay+(by-ay)*t+ny*bump} ${ax+(bx-ax)*u} ${ay+(by-ay)*u}`;
    }
  } return d+' Z';
}
function renderClouds() {
  $('overlay').replaceChildren(); if(!current)return;
  for(const [i,n] of [...current.annotations,...(draft?[draft]:[])].entries()) {
    const path=svg('path',{d:cloudPath(n),class:'cloud'+(n.id===selected?' selected':'')});
    if(n.id) {
      path.setAttribute('role','button');path.setAttribute('tabindex','0');path.setAttribute('aria-label',`Editar nube ${i+1}: ${n.comment}`);
      path.addEventListener('pointerdown',e=>{e.stopPropagation();openNote(n.id);});
      path.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();openNote(n.id);}});
    }
    $('overlay').append(path);
    if(n.id) {
      const x=n.x*current.width,y=n.y*current.height, r=11/scale;
      $('overlay').append(svg('circle',{cx:x,cy:y,r,class:'cloud-badge'}));
      const text=svg('text',{x,y:y+4/scale,'text-anchor':'middle',class:'cloud-label',style:`font-size:${12/scale}px`}); text.textContent=i+1; $('overlay').append(text);
    }
  }
}
$('overlay').addEventListener('pointerdown', e => {if(!current||historical||mode!=='draw'||e.button!==0)return; start=point(e); $('overlay').setPointerCapture(e.pointerId);});
$('overlay').addEventListener('pointermove', e => {if(!start)return; draft=rectangle(start,point(e));renderClouds();});
$('overlay').addEventListener('pointerup', e => {
  if(!start)return; const rect=rectangle(start,point(e));start=null;draft=null;renderClouds();
  if(rect.w*current.width*scale<12||rect.h*current.height*scale<12)return;
  editing={id:crypto.randomUUID(),...rect,comment:'',category:'general',priority:'normal',expected:'',lesson:''};openEditor(true);
});
$('overlay').addEventListener('pointercancel',()=>{start=null;draft=null;renderClouds();});
function openNote(id) { selected=id;renderClouds();if(historical)return;editing=structuredClone(current.annotations.find(n=>n.id===id));openEditor(false); }
function openEditor(isNew) {
  for(const k of ['comment','category','priority','expected','lesson']) $(k).value=editing[k];
  $('delete').hidden=isNew; $('noteNumber').textContent=isNew?current.annotations.length+1:current.annotations.findIndex(n=>n.id===editing.id)+1;
  const [x0,y0,x1,y1]=pixels(editing);$('coords').textContent=`Zona: [${x0}, ${y0}, ${x1}, ${y1}) · ${x1-x0} × ${y1-y0} px`;
  $('editor').showModal();setTimeout(()=>$('comment').focus(),30);
}
$('editor').querySelector('form').onsubmit = async e => {
  e.preventDefault(); if(!$('comment').value.trim()) return $('comment').focus();
  for(const k of ['comment','category','priority','expected','lesson']) editing[k]=$(k).value;
  const i=current.annotations.findIndex(n=>n.id===editing.id);if(i<0)current.annotations.push(editing);else current.annotations[i]=editing;
  selected=editing.id;$('editor').close();renderNotes();await persist();
};
$('cancel').onclick=()=>$('editor').close();
$('delete').onclick=async()=>{current.annotations=current.annotations.filter(n=>n.id!==editing.id);selected=null;$('editor').close();renderNotes();await persist();};
function renderNotes() {
  $('count').textContent=current.annotations.length; $('compactCount').textContent=current.annotations.length; $('notes').replaceChildren();
  for(const [i,n] of current.annotations.entries()) {
    const b=el('button','note'),top=el('div','note-top');top.append(el('span','number',String(i+1).padStart(2,'0')),el('span','',categories[n.category]||n.category));
    const [x0,y0,x1,y1]=pixels(n);b.append(top,el('p','',n.comment||'Sin comentario'),el('small','',`${x1-x0} × ${y1-y0} px · ${n.priority}`));
    b.onclick=()=>openNote(n.id);$('notes').append(b);
  } renderClouds();
}
function persist() {
  if(!current||historical)return Promise.resolve();
  const snapshot=structuredClone(current);changed=true;$('saved').textContent='Guardando…';
  const task=saveChain.then(async()=>{
    const latest=assets.find(a=>a.id===snapshot.id);snapshot.revision=latest.revision;
    const saved=await api('/api/save',snapshot);assets[assets.findIndex(a=>a.id===saved.id)]=saved;
    if(current?.id===saved.id)current.revision=saved.revision;
    changed=false;await updateRegistry();libraryTab=['approved','honors'].includes(current.status)?'accepted':'pending';gallery();showRevisions();$('saved').textContent='Guardado en este PC';return saved;
  });
  saveChain=task;task.catch(e=>{$('saved').textContent='Error al guardar';toast(e.message+' No cierres esta página.');});return task;
}
function flushGeneral() {if(generalTimer){clearTimeout(generalTimer);generalTimer=null;return persist();}return saveChain;}
$('general').oninput=()=>{if(!current)return;current.general=$('general').value;changed=true;clearTimeout(generalTimer);generalTimer=setTimeout(()=>{generalTimer=null;persist();},500);};
$('status').onchange=()=>{if(!current)return;current.status=$('status').value;persist();};
window.addEventListener('beforeunload',e=>{if(changed){e.preventDefault();e.returnValue='';}});
$('submit').onclick=async()=>{
  $('submit').disabled=true;
  try {
    await flushGeneral();await saveChain;
    const dirty=assets.filter(a=>a.revision!==a.submitted_revision&&(a.annotations.length||a.general.trim()||a.status!=='pending'));
    if(!dirty.length)throw Error('No hay comentarios nuevos para enviar.');
    const result=await api('/api/submit',{ids:dirty.map(a=>a.id),revisions:Object.fromEntries(dirty.map(a=>[a.id,a.revision]))});
    lastPath=result.path;$('receiptPath').textContent=result.path;$('openTxt').href=result.url;$('receipt').showModal();
    if(!$('openMd')){const link=el('a','button','Ver resumen MD ↗');link.id='openMd';link.target='_blank';$('openTxt').after(link);}$('openMd').href=result.markdown_url;
    for(const a of dirty)a.submitted_revision=a.revision;
  }catch(e){toast(e.message);}finally{$('submit').disabled=false;}
};
$('closeReceipt').onclick=()=>$('receipt').close();$('copyPath').onclick=async()=>{try{await navigator.clipboard.writeText(lastPath);$('copyPath').textContent='Ruta copiada';}catch{toast('Selecciona y copia la ruta mostrada.');}};
function upload() {
  $('upload').querySelector('form').reset();$('parent').replaceChildren(el('option','','Primera versión / sin vínculo'));$('parent').firstChild.value='';
  for(const a of assets){const o=el('option','',a.title+' · '+a.version);o.value=a.id;$('parent').append(o);}
  $('upload').showModal();
}
$('import').onclick=upload;$('import2').onclick=upload;$('cancelUpload').onclick=()=>$('upload').close();
$('file').onchange=()=>{if(!$('uploadTitle').value)$('uploadTitle').value=$('file').files[0]?.name.replace(/\.[^.]+$/,'')||'';};
$('upload').querySelector('form').onsubmit=async e=>{
  e.preventDefault();const f=$('file').files[0];if(!f)return;if(f.size>30*1024*1024)return toast('Máximo 30 MB por imagen');
  const button=e.submitter;button.disabled=true;
  try{
    const base64=await new Promise((resolve,reject)=>{const r=new FileReader();r.onload=()=>resolve(r.result.split(',')[1]);r.onerror=reject;r.readAsDataURL(f);});
    const a=await api('/api/import',{base64,title:$('uploadTitle').value,version:$('uploadVersion').value,parent_id:$('parent').value||null});
    if(!assets.some(x=>x.id===a.id))assets.push(a);$('upload').close();await choose(a.id);
  }catch(e){toast(e.message);}finally{button.disabled=false;}
};
function paintVerdict(){for(const b of document.querySelectorAll('[data-verdict]'))b.classList.toggle('active',b.dataset.verdict===chosenVerdict);}
$('rate').onclick=()=>{if(!current||historical)return;chosenVerdict=current.status;paintVerdict();$('ratingTitle').textContent=current.title;$('ratingComment').value='';$('rating').showModal();};
for(const b of document.querySelectorAll('[data-verdict]'))b.onclick=()=>{chosenVerdict=b.dataset.verdict;paintVerdict();};
$('closeRating').onclick=()=>$('rating').close();
$('saveRating').onclick=async()=>{current.status=chosenVerdict;const comment=$('ratingComment').value.trim();if(comment)current.general+=(current.general?'\n\n':'')+'[Valoración: '+labels[chosenVerdict]+'] '+comment;$('status').value=chosenVerdict;$('general').value=current.general;$('rating').close();await persist();};
$('history').onclick=async()=>{
  try{
    await flushGeneral();await saveChain;const state=await api('/api/state');assets=state.assets;approvals=state.approvals||[];
    const family=approvals.find(f=>f.version_ids.includes(current.id));$('familyVersions').replaceChildren();
    for(const id of family?.version_ids||[current.id]){
      const a=assets.find(x=>x.id===id);const b=el('button','history-entry',a.version+' · '+labels[a.status]+(id===family?.build_version?' · VIGENTE EN BUILD':''));
      b.onclick=()=>{$('historyDialog').close();choose(id);};$('familyVersions').append(b);
    }
    const result=await api('/api/history?id='+encodeURIComponent(current.id));$('historyEntries').replaceChildren();
    for(const e of result.events){
      const b=el('button','history-entry'),r=e.review;b.append(el('strong','',new Date(e.at).toLocaleString('es-CL')+' · revisión '+r.revision),el('small','',labels[r.status]+' · '+r.annotations.length+' nubes'),el('p','',r.annotations.map(n=>n.comment).join(' / ')||r.general||'Imagen recién importada'));
      b.onclick=()=>{
        current=structuredClone(r);historical=true;$('historyBanner').hidden=false;$('status').value=r.status;$('general').value=r.general;
        for(const k of ['status','general','draw','submit','rate'])$(k).disabled=true;
        $('historyDialog').close();setMode('select');renderNotes();showApproval();
      };$('historyEntries').append(b);
    }$('historyDialog').showModal();
  }catch(e){toast(e.message);}
};
$('closeHistory').onclick=()=>$('historyDialog').close();$('backLive').onclick=()=>choose(current.id);
function latestDescendant(id, list=assets){
  const descendants=new Set([id]);let grew=true;
  while(grew){grew=false;for(const a of list)if(descendants.has(a.parent_id)&&!descendants.has(a.id)){descendants.add(a.id);grew=true;}}
  return list.filter(a=>descendants.has(a.id)).at(-1)?.id||id;
}
function safeToUpdate(){return !historical&&!changed&&!generalTimer&&!start&&!document.querySelector('dialog[open]')&&!['INPUT','TEXTAREA','SELECT'].includes(document.activeElement?.tagName);}
async function applyPendingUpdate(){
  if(!pendingUpdate)return;
  if(!safeToUpdate())return toast('Termina o guarda el comentario abierto; la actualización queda esperando.');
  const update=pendingUpdate;pendingUpdate=null;$('updates').hidden=true;
  if(update.kind==='app')return location.reload();
  const address=new URL(location.href);address.searchParams.set('image',update.id);address.searchParams.set('delivery',update.id);address.searchParams.delete('view');location.replace(address.href);
}
$('applyUpdate').onclick=applyPendingUpdate;
async function checkUpdates(){
  if(checkingUpdate||!current)return;checkingUpdate=true;
  try{
    const state=await api('/api/state');
    if(loadedBuild&&state.disk_build&&state.disk_build!==loadedBuild){
      pendingUpdate={kind:'app'};$('updateMessage').textContent='Hay una actualización de la app. Tus comentarios están conservados.';
      $('applyUpdate').textContent='Actualizar app';$('updates').hidden=false;
      if(safeToUpdate())await applyPendingUpdate();return;
    }
    const latest=latestDescendant(current.id,state.assets);
    if(latest!==current.id){
      pendingUpdate={kind:'image',id:latest,state};$('updateMessage').textContent='Hay una propuesta más reciente de esta pieza. La anterior sigue en el historial.';
      $('applyUpdate').textContent='Mostrar nueva versión';$('updates').hidden=false;
      if(followLatest&&safeToUpdate())await applyPendingUpdate();
    }else if(safeToUpdate()&&(state.assets.length!==assets.length||state.assets.some(a=>{const old=assets.find(x=>x.id===a.id);return old&&(old.revision!==a.revision||old.status!==a.status);}))) {
      const previousStatus=current.status;assets=state.assets;approvals=state.approvals||[];current=assets.find(a=>a.id===current.id)||current;
      if(current.status!==previousStatus)libraryTab=['approved','honors'].includes(current.status)?'accepted':'pending';
      $('status').value=current.status;$('general').value=current.general;gallery();renderNotes();showApproval();showRevisions();
    }
  }catch{/* A launcher restart can briefly make the server unavailable. Retry without touching edits. */}
  finally{checkingUpdate=false;}
}
setInterval(checkUpdates,5000);window.addEventListener('focus',checkUpdates);
api('/api/state').then(state=>{
  document.title='Atlas '+state.app_version+' · Revisión de arte';
  assets=state.assets;approvals=state.approvals||[];loadedBuild=state.disk_build;
  const params=new URLSearchParams(location.search),requested=params.get('image'),remembered=localStorage.getItem('atlas-selected');
  if(assets.length){
    let id=assets.find(a=>a.id===(requested||remembered))?.id||assets.at(-1).id;
    const historicalView=params.get('view')==='history';
    id=params.get('latest')==='1'?(state.latest_delivery_id||assets.at(-1).id):(historicalView?id:latestDescendant(id));
    choose(id,{follow:!historicalView});
  }else{$('empty').hidden=false;$('paper').hidden=true;$('saved').textContent='Listo para importar';}
}).catch(e=>toast(e.message));
