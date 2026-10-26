const $ = id => document.getElementById(id);
let assets = [], approvals = [], queue = [], current = null, mode = 'draw', start = null, draft = null, editing = null, selected = null;
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
const labels = {pending:'Pendiente de revisión', changes_requested:'Aprobado (mejorar)', approved:'Aprobado', honors:'★ Aprobado con honores'};
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
/* the library's tabs (the user, 24/10): Pre = waiting for your review, Rev = you sent clouds and wait for the new
   version (so they are not reviewed twice), Aprobado = Aprobados / Honores. Each piece is in one tab. */
let acceptedSub='approved';
const accepted=a=>a&&(a.status==='approved'||a.status==='honors'||a.status==='changes_requested'); /* mejorar: approved too */
const hasFeedback=a=>a&&(a.annotations.length||(a.doodles||[]).length||a.general.trim());
function pieceGroups(){
  const byId=new Map(assets.map(a=>[a.id,a])),groups=new Map();
  for(const a of assets){let root=a;const seen=new Set();while(root.parent_id&&byId.has(root.parent_id)&&!seen.has(root.id)){seen.add(root.id);root=byId.get(root.parent_id);}if(!groups.has(root.id))groups.set(root.id,{root,versions:[]});groups.get(root.id).versions.push(a);}
  for(const g of groups.values()){
    const family=approvals.find(f=>f.family_id===g.root.id),latest=g.versions.at(-1),turn=queue.find(q=>q.family_id===g.root.id);
    g.turn=turn;
    if(accepted(latest)){
      const honors=family?.build_version&&byId.get(family.build_version);
      /* approved but still being improved (its grade, or comments not answered yet) goes under Aprobado (mejorar) */
      const improving=latest.status==='changes_requested'||!!turn?.unanswered?.length;
      g.tab='accepted';g.sub=improving?'improve':latest.status==='honors'?'honors':'approved';
      g.shown=byId.get(family?.build_version)||byId.get(family?.approved_version)||latest;
    }else{
      const sent=latest.submitted_revision!=null&&latest.submitted_revision===latest.revision&&hasFeedback(latest);
      g.tab=(turn?.turn==='agente'&&turn.unanswered.length)||sent?'review':'pending';g.shown=latest;
    }
  }
  return groups;
}
function tabOf(id){for(const g of pieceGroups().values())if(g.versions.some(v=>v.id===id))return g;return null;}
function gallery() {
  $('gallery').replaceChildren();
  $('assetPicker').replaceChildren();
  const search=$('searchPieces').value.trim().toLocaleLowerCase('es');let total=0;const counts={pending:0,review:0,accepted:0,improve:0,approved:0,honors:0};
  for (const {root,versions,tab,sub,shown:a,turn} of pieceGroups().values()) {
    counts[tab]++;if(tab==='accepted')counts[sub]++;
    if(tab!==libraryTab||tab==='accepted'&&sub!==acceptedSub)continue;
    if(search&&!(`${root.title} ${a.version}`).toLocaleLowerCase('es').includes(search))continue;total++;
    const option=el('option','',root.title+' · '+a.version);option.value=a.id;option.selected=a.id===current?.id;$('assetPicker').append(option);
    const button = el('button','asset' + (versions.some(v=>v.id===current?.id) ? ' active' : ''));
    const img = el('img'); img.src = a.image_url;img.loading='lazy';img.alt = ''; button.append(img,el('strong','',root.title),el('small',a.status==='honors'?'honors':'',labels[a.status]+(sub==='improve'?' · en el juego mientras se mejora':'')),el('small','',a.version),el('small','',versions.length+' revisión(es) · ver Historial'));
    if(tab==='pending')button.append(el('small','turn you','● Te toca'));
    if(tab==='review')button.append(el('small','turn agent','◌ Enviado · esperando versión nueva'+(turn?.unanswered.length?` (${turn.unanswered.length} envío(s))`:'')));
    button.onclick = () => choose(a.id,{follow:tab!=='accepted'}); $('gallery').append(button);
  }
  if(current&&!Array.from($('assetPicker').options).some(o=>o.value===current.id)){
    const pinned=el('option','',`En revisión: ${current.title} · ${labels[current.status]} (fuera del filtro)`);pinned.value=current.id;pinned.selected=true;$('assetPicker').prepend(pinned);
  }
  if(!total)$('gallery').append(el('p','subtle','No hay piezas en esta pestaña.'));
  $('tabPending').textContent=`Pre (${counts.pending})`;$('tabReview').textContent=`Rev (${counts.review})`;$('tabAccepted').textContent=`Aprobado (${counts.accepted})`;
  $('tabImprove').textContent=`Aprobado (mejorar) (${counts.improve})`;$('tabApproved').textContent=`Aprobados (${counts.approved})`;$('tabHonors').textContent=`★ Honores (${counts.honors})`;
  for(const [id,on] of [['tabPending',libraryTab==='pending'],['tabReview',libraryTab==='review'],['tabAccepted',libraryTab==='accepted'],['tabImprove',acceptedSub==='improve'],['tabApproved',acceptedSub==='approved'],['tabHonors',acceptedSub==='honors']])$(id).setAttribute('aria-selected',String(on));
  $('acceptedTabs').hidden=libraryTab!=='accepted';
}
function setLibraryTab(value,sub){libraryTab=value;if(sub)acceptedSub=sub;gallery();}
function tabForPiece(id){const g=tabOf(id);if(g){libraryTab=g.tab;if(g.sub)acceptedSub=g.sub;}}
$('tabPending').onclick=()=>setLibraryTab('pending');$('tabReview').onclick=()=>setLibraryTab('review');$('tabAccepted').onclick=()=>setLibraryTab('accepted');
$('tabImprove').onclick=()=>setLibraryTab('accepted','improve');$('tabApproved').onclick=()=>setLibraryTab('accepted','approved');$('tabHonors').onclick=()=>setLibraryTab('accepted','honors');$('searchPieces').oninput=gallery;
async function choose(id, options={}) {
  try { await flushGeneral(); await saveChain; } catch(e) { return toast(e.message); }
  current = assets.find(a => a.id === id); selected = null;
  /* the tab stays the one the reviewer is on (moving between revisions of an accepted piece used to jump to
     Preliminares); only the first load picks it from the piece */
  if(options.tab)tabForPiece(id);
  undoStack=[];if(current&&!current.doodles)current.doodles=[];
  /* an update notice was for the piece shown before: it must not take this one elsewhere */
  if(pendingUpdate&&pendingUpdate.kind==='image'&&pendingUpdate.forId!==id){pendingUpdate=null;$('updates').hidden=true;}
  followLatest=options.follow===true;
  const address=new URL(location.href);address.searchParams.set('image',id);address.searchParams.delete('latest');history.replaceState(null,'',address);
  if(followLatest)address.searchParams.delete('view');else address.searchParams.set('view','history');history.replaceState(null,'',address);
  historical=false;$('historyBanner').hidden=true;
  for(const k of ['status','general','draw','pen','eraser','submit','rate'])$(k).disabled=false;
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
async function updateRegistry(){const state=await api('/api/state');approvals=state.approvals||[];queue=state.queue||[];showApproval();}
/* zoom like a standard image viewer (the user, 24/10): the wheel zooms toward the cursor in steps, − / ＋ and the
   keys + − 0 (fit) 1 (1:1), a % box with presets, double click toggles fit / 1:1, drag with the hand (or Space,
   or the middle / right button) to move */
const zoomSteps=[5,10,15,20,25,33,50,67,75,100,125,150,200,300,400,500,600,800,1000,1200,1600,2000,2400,3200];
for(const v of zoomSteps){const o=el('option');o.value=v+'%';$('zoomSteps').append(o);}
function resize() {
  if (!current) return;
  $('paper').style.width = current.width * scale + 'px'; $('paper').style.height = current.height * scale + 'px';
  padPaper();
  if(document.activeElement!==$('zoomNumber'))$('zoomNumber').value=Math.round(scale*100)+'%'; renderClouds();
}
/* room to pan at any zoom (the user, 24/10: zoomed out it did not move): margins around the paper so it can travel
   until a little empty space (12 % of the view) shows past its edge, never further, wherever it is dragged; a paper
   smaller than the view travels from one edge of the view to the other */
function padPaper(){
  const v=$('viewport'),cw=v.clientWidth,ch=v.clientHeight,pw=current.width*scale,ph=current.height*scale;
  const mx=Math.round(Math.max(cw*.12,cw-cw*.12-pw)),my=Math.round(Math.max(ch*.12,ch-ch*.12-ph));
  $('paper').style.margin=my+'px '+mx+'px';
}
function centerPaper(){const v=$('viewport');v.scrollLeft=(v.scrollWidth-v.clientWidth)/2;v.scrollTop=(v.scrollHeight-v.clientHeight)/2;}
function fitScale(){return Math.max(.03,Math.min(($('viewport').clientWidth - 24) / current.width, ($('viewport').clientHeight - 24) / current.height, 8));}
function fit() { if (!current) return; fitMode=true; scale = fitScale(); resize(); centerPaper(); }
function zoomTo(value, anchor) {
  if(!current)return; fitMode=false;
  const viewport=$('viewport'),before=$('paper').getBoundingClientRect(),vr=viewport.getBoundingClientRect();
  const p=anchor||{x:vr.left+viewport.clientWidth/2,y:vr.top+viewport.clientHeight/2};
  const px=(p.x-before.left)/scale,py=(p.y-before.top)/scale;
  scale=Math.max(.05,Math.min(32,value));resize();
  const after=$('paper').getBoundingClientRect();viewport.scrollLeft+=after.left+px*scale-p.x;viewport.scrollTop+=after.top+py*scale-p.y;
}
function zoomStep(dir,anchor){
  const now=scale*100*(1+dir*1e-6);
  const next=dir>0?zoomSteps.find(v=>v>now):[...zoomSteps].reverse().find(v=>v<now);
  if(next)zoomTo(next/100,anchor);
}
$('fit').onclick = fit;
$('zoomNumber').onchange=()=>{const v=parseFloat($('zoomNumber').value);if(v>0)zoomTo(v/100);else resize();};
$('zoomNumber').onkeydown=e=>{if(e.key==='Enter'){$('zoomNumber').onchange();$('zoomNumber').blur();}if(e.key==='Escape'){$('zoomNumber').blur();resize();}};
$('zoomNumber').onfocus=()=>$('zoomNumber').select();$('zoomNumber').onblur=()=>resize();
$('fitWidth').onclick=()=>zoomTo(($('viewport').clientWidth-24)/current.width);
$('actual').onclick=()=>zoomTo(1);$('minus').onclick=()=>zoomStep(-1);$('plus').onclick=()=>zoomStep(1);
let wheelRest=0;
$('viewport').addEventListener('wheel',e=>{
  if(!current||e.shiftKey&&!e.ctrlKey)return; /* Shift + wheel scrolls sideways */
  e.preventDefault();
  if(e.ctrlKey&&!Number.isInteger(e.deltaY)){zoomTo(scale*Math.exp(-e.deltaY*.01),{x:e.clientX,y:e.clientY});return;} /* trackpad pinch: smooth */
  wheelRest+=e.deltaMode?e.deltaY*40:e.deltaY;if(Math.abs(wheelRest)<40)return;
  zoomStep(wheelRest<0?1:-1,{x:e.clientX,y:e.clientY});wheelRest=0;
},{passive:false});
$('viewport').addEventListener('dblclick',e=>{if(!current||mode==='draw'||mode==='pen'||mode==='eraser')return;if(fitMode||Math.abs(scale-fitScale())<.01)zoomTo(1,{x:e.clientX,y:e.clientY});else fit();});
window.addEventListener('keydown',e=>{
  if(document.querySelector('dialog[open]')||['INPUT','TEXTAREA','SELECT'].includes(document.activeElement?.tagName))return;
  if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='z'){e.preventDefault();undoDoodle();return;}
  if(e.ctrlKey||e.metaKey||e.altKey)return;
  const k=e.key;
  if(k==='+'||k==='=')zoomStep(1);else if(k==='-'||k==='_')zoomStep(-1);else if(k==='0')fit();else if(k==='1')zoomTo(1);
  else if(k==='n'||k==='N')setMode('draw');else if(k==='p'||k==='P')setMode('pen');else if(k==='e'||k==='E')setMode('eraser');
  else if(k==='h'||k==='H')setMode('hand');else if(k==='v'||k==='V')setMode('select');else return;
  e.preventDefault();
});
window.addEventListener('resize', ()=>{positionDeskControls();if(fitMode)fit();else resize();});
$('focus').onclick=()=>{document.body.classList.toggle('focus-mode');positionDeskControls();$('focus').textContent=document.body.classList.contains('focus-mode')?'Restaurar paneles ⛶':'Ampliar mesa ⛶';fit();};
let spaceHeld=false,pan=null;
window.addEventListener('keydown',e=>{if(e.code==='Space'&&!document.querySelector('dialog[open]')&&!['INPUT','TEXTAREA','SELECT','BUTTON'].includes(document.activeElement?.tagName)){spaceHeld=true;e.preventDefault();}});
const panViewport=$('viewport');
function endPan(e){
  if(!pan||(e?.pointerId!==undefined&&e.pointerId!==pan.pointerId))return;
  const pointerId=pan.pointerId;pan=null;panViewport.classList.remove('panning');
  if(panViewport.hasPointerCapture(pointerId))panViewport.releasePointerCapture(pointerId);
}
window.addEventListener('keyup',e=>{if(e.code==='Space')spaceHeld=false;});
window.addEventListener('blur',()=>{spaceHeld=false;endPan();});
panViewport.addEventListener('contextmenu',e=>e.preventDefault());
panViewport.addEventListener('pointerdown',e=>{
  if(pan||!(e.button===2||e.button===1||((spaceHeld||mode==='hand')&&e.button===0)))return;
  e.preventDefault();e.stopPropagation();
  pan={pointerId:e.pointerId,buttons:e.button===2?2:e.button===1?4:1,x:e.clientX,y:e.clientY,left:panViewport.scrollLeft,top:panViewport.scrollTop};
  panViewport.classList.add('panning');panViewport.setPointerCapture(e.pointerId);
},true);
panViewport.addEventListener('pointermove',e=>{
  if(!pan||e.pointerId!==pan.pointerId)return;
  e.preventDefault();e.stopPropagation();
  if(!(e.buttons&pan.buttons)){endPan(e);return;}
  panViewport.scrollLeft=Math.max(0,Math.min(panViewport.scrollWidth-panViewport.clientWidth,pan.left+pan.x-e.clientX));
  panViewport.scrollTop=Math.max(0,Math.min(panViewport.scrollHeight-panViewport.clientHeight,pan.top+pan.y-e.clientY));
},true);
for(const event of ['pointerup','pointercancel'])window.addEventListener(event,e=>{
  if(!pan||e.pointerId!==pan.pointerId)return;
  e.preventDefault();e.stopPropagation();endPan(e);
},true);
panViewport.addEventListener('lostpointercapture',endPan);
$('showNotes').onclick=()=>document.body.classList.toggle('notes-open');$('closeNotes').onclick=()=>document.body.classList.remove('notes-open');
$('assetPicker').onchange=()=>choose($('assetPicker').value);
$('refresh').onclick=async()=>{try{await flushGeneral();await saveChain;const state=await api('/api/state');assets=state.assets;approvals=state.approvals||[];queue=state.queue||[];await choose(current?.id||assets[0].id);toast('Versiones actualizadas');}catch(e){toast(e.message);}};
document.body.dataset.density='compact'; /* one interface size (the user, 24/10: the compact one, no selector) */
function setMode(value) {
  if(historical&&(value==='draw'||value==='pen'||value==='eraser'))value='select';
  mode=value;for(const k of ['draw','pen','eraser','hand','select'])$(k).classList.toggle('active',k===value);
  $('overlay').classList.toggle('selecting',value==='select');$('overlay').dataset.mode=value;$('viewport').dataset.mode=value;
  $('penTools').classList.toggle('on',value==='pen'||value==='eraser');
  $('toolHint').textContent={draw:'Arrastra una zona para comentarla',pen:'Dibuja a mano alzada · Ctrl+Z deshace',eraser:'Pasa sobre un trazo para borrarlo',hand:'Arrastra para mover · doble clic: ajustar / 1:1',select:'Toca una nube para editarla'}[value]+' · botón derecho: mover · rueda: zoom';
}
$('draw').onclick = () => setMode('draw'); $('select').onclick = () => setMode('select');$('pen').onclick=()=>setMode('pen');$('eraser').onclick=()=>setMode('eraser');$('hand').onclick=()=>setMode('hand');
/* pen strokes over the image (the user, 24/10: like Windows' screenshot markup, eight colours): stored per version,
   normalized points, the width in image pixels (the screen width chosen, at the zoom it was drawn) */
let penColor='#e53935',penWidth=5,stroke=null,undoStack=[];
try{penColor=localStorage.getItem('atlas-pen-color')||penColor;penWidth=Number(localStorage.getItem('atlas-pen-width'))||penWidth;}catch{}
function remember(k,v){try{localStorage.setItem(k,v);}catch{}}
function paintPen(){for(const b of document.querySelectorAll('.swatch'))b.classList.toggle('active',b.dataset.color===penColor);for(const b of document.querySelectorAll('[data-width]'))b.classList.toggle('active',Number(b.dataset.width)===penWidth);}
for(const b of document.querySelectorAll('.swatch'))b.onclick=()=>{penColor=b.dataset.color;remember('atlas-pen-color',penColor);paintPen();if(mode!=='pen')setMode('pen');};
for(const b of document.querySelectorAll('[data-width]'))b.onclick=()=>{penWidth=Number(b.dataset.width);remember('atlas-pen-width',penWidth);paintPen();if(mode!=='pen')setMode('pen');};
paintPen();
function strokePath(d){const w=current.width,h=current.height,p=d.points;if(p.length===1)return `M ${p[0][0]*w} ${p[0][1]*h} l 0.001 0`;let s=`M ${p[0][0]*w} ${p[0][1]*h}`;
  for(let i=1;i<p.length-1;i++){const mx=(p[i][0]+p[i+1][0])/2*w,my=(p[i][1]+p[i+1][1])/2*h;s+=` Q ${p[i][0]*w} ${p[i][1]*h} ${mx} ${my}`;}
  const l=p.at(-1);return s+` L ${l[0]*w} ${l[1]*h}`;}
function eraseDoodle(id){const i=current.doodles.findIndex(d=>d.id===id);if(i<0)return;undoStack.push({erase:current.doodles[i],at:i});current.doodles.splice(i,1);renderNotes();persist();}
function undoDoodle(){if(historical||!current)return;const u=undoStack.pop();if(!u)return;if(u.add)current.doodles=current.doodles.filter(d=>d.id!==u.add);else current.doodles.splice(Math.min(u.at,current.doodles.length),0,u.erase);renderNotes();persist();}
$('undo').onclick=undoDoodle;
$('clearDoodles').onclick=()=>{if(historical||!current?.doodles.length)return;if(!confirm('¿Borrar los '+current.doodles.length+' trazos de esta versión? (Ctrl+Z los recupera)'))return;for(let i=current.doodles.length-1;i>=0;i--)undoStack.push({erase:current.doodles[i],at:i});current.doodles=[];renderNotes();persist();};
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
  for(const d of [...(current.doodles||[]),...(stroke?[stroke]:[])]){
    const path=svg('path',{d:strokePath(d),class:'doodle',stroke:d.color,'stroke-width':d.width});
    if(d!==stroke){path.addEventListener('pointerdown',e=>{if(mode==='eraser'&&!historical){e.stopPropagation();eraseDoodle(d.id);}});
      path.addEventListener('pointerenter',e=>{if(mode==='eraser'&&!historical&&e.buttons&1)eraseDoodle(d.id);});}
    $('overlay').append(path);
  }
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
$('overlay').addEventListener('pointerdown', e => {
  if(!current||historical||e.button!==0||spaceHeld)return;
  if(mode==='pen'){const p=point(e);stroke={id:crypto.randomUUID(),color:penColor,width:+(penWidth/scale).toFixed(3),points:[[p.x,p.y]]};$('overlay').setPointerCapture(e.pointerId);renderClouds();return;}
  if(mode!=='draw')return; start=point(e); $('overlay').setPointerCapture(e.pointerId);});
$('overlay').addEventListener('pointermove', e => {
  if(stroke){const p=point(e),l=stroke.points.at(-1),r=$('overlay').getBoundingClientRect();if(Math.hypot((p.x-l[0])*r.width,(p.y-l[1])*r.height)<1.5)return;
    if(stroke.points.length<5000)stroke.points.push([p.x,p.y]);renderClouds();return;}
  if(!start)return; draft=rectangle(start,point(e));renderClouds();});
$('overlay').addEventListener('pointerup', e => {
  if(stroke){const d=stroke;stroke=null;d.points=d.points.map(([x,y])=>[+x.toFixed(5),+y.toFixed(5)]);current.doodles.push(d);undoStack.push({add:d.id});renderNotes();persist();return;}
  if(!start)return; const rect=rectangle(start,point(e));start=null;draft=null;renderClouds();
  if(rect.w*current.width*scale<12||rect.h*current.height*scale<12)return;
  editing={id:crypto.randomUUID(),...rect,comment:'',category:'general',priority:'normal',expected:'',lesson:''};openEditor(true);
});
$('overlay').addEventListener('pointercancel',()=>{start=null;draft=null;stroke=null;renderClouds();});
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
  }
  const nd=(current.doodles||[]).length;if(nd){const b=el('div','note doodles');b.append(el('div','note-top','✎ Trazos de lápiz'),el('p','',nd+' trazo(s) sobre la imagen: van en el envío (imagen con garabatos y sus cajas).'));$('notes').append(b);}
  renderClouds();
}
function persist() {
  if(!current||historical)return Promise.resolve();
  const snapshot=structuredClone(current);changed=true;$('saved').textContent='Guardando…';
  const task=saveChain.then(async()=>{
    const latest=assets.find(a=>a.id===snapshot.id);snapshot.revision=latest.revision;
    const saved=await api('/api/save',snapshot);assets[assets.findIndex(a=>a.id===saved.id)]=saved;
    if(current?.id===saved.id)current.revision=saved.revision;
    changed=false;await updateRegistry();gallery();showRevisions();$('saved').textContent='Guardado en este PC';return saved;
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
    const dirty=assets.filter(a=>a.revision!==a.submitted_revision&&(hasFeedback(a)||a.status!=='pending'));
    if(!dirty.length)throw Error('No hay comentarios nuevos para enviar.');
    const result=await api('/api/submit',{ids:dirty.map(a=>a.id),revisions:Object.fromEntries(dirty.map(a=>[a.id,a.revision]))});
    lastPath=result.path;$('receiptPath').textContent=result.path;$('openTxt').href=result.url;$('receipt').showModal();
    if(!$('openMd')){const link=el('a','button','Ver resumen MD ↗');link.id='openMd';link.target='_blank';$('openTxt').after(link);}$('openMd').href=result.markdown_url;
    for(const a of dirty)a.submitted_revision=a.revision;
    await updateRegistry();gallery();
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
    await flushGeneral();await saveChain;const state=await api('/api/state');assets=state.assets;approvals=state.approvals||[];queue=state.queue||[];
    const family=approvals.find(f=>f.version_ids.includes(current.id));$('familyVersions').replaceChildren();
    for(const id of family?.version_ids||[current.id]){
      const a=assets.find(x=>x.id===id);const b=el('button','history-entry',a.version+' · '+labels[a.status]+(id===family?.build_version?' · VIGENTE EN BUILD':''));
      b.onclick=()=>{$('historyDialog').close();choose(id);};$('familyVersions').append(b);
    }
    const result=await api('/api/history?id='+encodeURIComponent(current.id));$('historyEntries').replaceChildren();
    for(const e of result.events){
      const b=el('button','history-entry'),r=e.review;b.append(el('strong','',new Date(e.at).toLocaleString('es-CL')+' · revisión '+r.revision),el('small','',labels[r.status]+' · '+r.annotations.length+' nubes'),el('p','',r.annotations.map(n=>n.comment).join(' / ')||r.general||'Imagen recién importada'));
      b.onclick=()=>{
        current=structuredClone(r);if(!current.doodles)current.doodles=[];historical=true;$('historyBanner').hidden=false;$('status').value=r.status;$('general').value=r.general;
        for(const k of ['status','general','draw','pen','eraser','submit','rate'])$(k).disabled=true;
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
function safeToUpdate(){return !historical&&!changed&&!generalTimer&&!start&&!stroke&&!document.querySelector('dialog[open]')&&!['INPUT','TEXTAREA','SELECT'].includes(document.activeElement?.tagName);}
async function applyPendingUpdate(){
  if(!pendingUpdate)return;
  if(!safeToUpdate())return toast('Termina o guarda el comentario abierto; la actualización queda esperando.');
  const update=pendingUpdate;pendingUpdate=null;$('updates').hidden=true;
  if(update.kind==='app')return location.reload();
  if(update.forId!==current?.id)return; /* stale: computed for another piece */
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
      pendingUpdate={kind:'image',id:latest,forId:current.id,state};$('updateMessage').textContent='Hay una propuesta más reciente de esta pieza. La anterior sigue en el historial.';
      $('applyUpdate').textContent='Mostrar nueva versión';$('updates').hidden=false;
      if(followLatest&&safeToUpdate())await applyPendingUpdate();
    }else if(safeToUpdate()&&(state.assets.length!==assets.length||state.assets.some(a=>{const old=assets.find(x=>x.id===a.id);return old&&(old.revision!==a.revision||old.status!==a.status);}))) {
      const previousStatus=current.status;assets=state.assets;approvals=state.approvals||[];queue=state.queue||[];current=assets.find(a=>a.id===current.id)||current;
      $('status').value=current.status;$('general').value=current.general;gallery();renderNotes();showApproval();showRevisions();
    }
  }catch{/* A launcher restart can briefly make the server unavailable. Retry without touching edits. */}
  finally{checkingUpdate=false;}
}
setInterval(checkUpdates,5000);window.addEventListener('focus',checkUpdates);
api('/api/state').then(state=>{
  document.title='Atlas '+state.app_version+' · Revisión de arte';
  assets=state.assets;approvals=state.approvals||[];queue=state.queue||[];loadedBuild=state.disk_build;
  const params=new URLSearchParams(location.search),requested=params.get('image'),remembered=localStorage.getItem('atlas-selected');
  if(assets.length){
    let id=assets.find(a=>a.id===(requested||remembered))?.id||assets.at(-1).id;
    const historicalView=params.get('view')==='history';
    id=params.get('latest')==='1'?(state.latest_delivery_id||assets.at(-1).id):(historicalView?id:latestDescendant(id));
    const fam=state.approvals?.find(f=>f.version_ids.includes(id)),chosen=assets.find(a=>a.id===id);
    choose(id,{follow:!historicalView,tab:true});
  }else{$('empty').hidden=false;$('paper').hidden=true;$('saved').textContent='Listo para importar';}
}).catch(e=>toast(e.message));

/* Pixel painter: edits an exact PNG copy; publishing adds an Atlas child. */
(()=>{
const entry=document.getElementById('pixelPaint');if(!entry)return;
const dialog=document.createElement('dialog');dialog.id='pixelPainter';
dialog.innerHTML=`<div class="pixel-head"><strong id="pixelTitle">Píxeles</strong><button id="pixelClose" aria-label="Cerrar pintor">Cerrar</button></div>
<div class="pixel-controls"><label>Herramienta <select id="pixelTool"><option value="pen">Lápiz</option><option value="pick">Cuentagotas</option><option value="erase">Borrador</option></select></label><label>Color <input type="color" id="pixelColor" value="#55a544"></label><label>Cuadrícula nativa <input id="pixelStep" type="number" min="1" max="32" value="1"> px</label><label>Zoom <select id="pixelZoom"></select></label><label><input type="checkbox" id="pixelGrid" checked> Ver cuadrícula</label><button id="pixelUndo" disabled>Deshacer</button><span id="pixelPosition" role="status"></span></div>
<div id="pixelPalette" aria-label="Colores de la imagen"></div><div id="pixelViewport"><div id="pixelSurface"><canvas id="pixelCanvas"></canvas><div id="pixelOverlay"></div></div></div>
<div class="pixel-footer"><span>Guardar crea una versión nueva. Botón derecho: tomar color · Ctrl+Z: deshacer.</span><button id="pixelDownload">Descargar PNG</button><button class="primary" id="pixelSave">Guardar en Atlas</button></div>`;
document.body.append(dialog);
const q=id=>dialog.querySelector('#'+id),canvas=q('pixelCanvas'),ctx=canvas.getContext('2d',{willReadFrequently:true});
let parent=null,zoom=1,stroke=null,last=null,undo=[],dirty=false;
for(const z of [.125,.25,.5,1,2,4,8,16]){const o=document.createElement('option');o.value=z;o.textContent=z<1?`${z*100}%`:`${z}×`;q('pixelZoom').append(o);}
function layout(){zoom=+q('pixelZoom').value;canvas.style.width=canvas.width*zoom+'px';canvas.style.height=canvas.height*zoom+'px';q('pixelOverlay').style.backgroundSize=`${zoom*+q('pixelStep').value}px ${zoom*+q('pixelStep').value}px`;q('pixelOverlay').hidden=!q('pixelGrid').checked||zoom<4;}
q('pixelZoom').onchange=layout;q('pixelGrid').onchange=layout;q('pixelStep').onchange=()=>{q('pixelStep').value=Math.min(32,Math.max(1,Math.round(+q('pixelStep').value||1)));layout();};
function point(e){const r=canvas.getBoundingClientRect();return{x:Math.min(canvas.width-1,Math.max(0,Math.floor((e.clientX-r.left)/zoom))),y:Math.min(canvas.height-1,Math.max(0,Math.floor((e.clientY-r.top)/zoom)))};}
function pick(p){const d=ctx.getImageData(p.x,p.y,1,1).data;q('pixelColor').value='#'+[...d.slice(0,3)].map(v=>v.toString(16).padStart(2,'0')).join('');}
function cell(x,y){const step=+q('pixelStep').value;x*=step;y*=step;if(x<0||y<0||x>=canvas.width||y>=canvas.height)return;const key=x+':'+y,w=Math.min(step,canvas.width-x),h=Math.min(step,canvas.height-y);if(!stroke.has(key))stroke.set(key,{x,y,data:ctx.getImageData(x,y,w,h)});if(q('pixelTool').value==='erase')ctx.clearRect(x,y,w,h);else{ctx.fillStyle=q('pixelColor').value;ctx.fillRect(x,y,w,h);}dirty=true;}
function draw(p){const step=+q('pixelStep').value,b={x:Math.floor(p.x/step),y:Math.floor(p.y/step)},a=last||b;let x=a.x,y=a.y;const dx=Math.abs(b.x-x),dy=-Math.abs(b.y-y),sx=x<b.x?1:-1,sy=y<b.y?1:-1;let err=dx+dy;while(true){cell(x,y);if(x===b.x&&y===b.y)break;const e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}last=b;}
canvas.oncontextmenu=e=>e.preventDefault();
canvas.onpointerdown=e=>{if(e.button!==0&&e.button!==2)return;e.preventDefault();const p=point(e);if(e.button===2||q('pixelTool').value==='pick'){pick(p);return;}stroke=new Map();last=null;canvas.setPointerCapture(e.pointerId);draw(p);};
canvas.onpointermove=e=>{const p=point(e);q('pixelPosition').textContent=`${p.x}, ${p.y} · ${canvas.width} × ${canvas.height}`;if(stroke)draw(p);};
function finish(){if(stroke?.size){undo.push([...stroke.values()]);if(undo.length>32)undo.shift();q('pixelUndo').disabled=false;}stroke=null;last=null;}
canvas.onpointerup=finish;canvas.onpointercancel=finish;
q('pixelUndo').onclick=()=>{finish();const patches=undo.pop();if(patches)for(const p of patches)ctx.putImageData(p.data,p.x,p.y);q('pixelUndo').disabled=!undo.length;dirty=!!undo.length;};
dialog.addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='z'){e.preventDefault();q('pixelUndo').click();}});
q('pixelClose').onclick=()=>{finish();dialog.close();};dialog.addEventListener('cancel',finish);
entry.onclick=async()=>{if(!current)return toast('Elige una imagen para pintar.');try{if(parent?.id!==current.id){const a=current,img=new Image();img.src=a.image_url;await img.decode();parent={id:a.id,title:a.title};canvas.width=img.naturalWidth;canvas.height=img.naturalHeight;ctx.imageSmoothingEnabled=false;ctx.drawImage(img,0,0);undo=[];dirty=false;q('pixelUndo').disabled=true;q('pixelTitle').textContent=a.title+' · Píxeles';const data=ctx.getImageData(0,0,canvas.width,canvas.height).data,counts=new Map();for(let i=0;i<data.length;i+=4){if(!data[i+3])continue;const key=(data[i]<<16)|(data[i+1]<<8)|data[i+2];counts.set(key,(counts.get(key)||0)+1);}q('pixelPalette').replaceChildren();for(const [rgb]of [...counts].sort((a,b)=>b[1]-a[1]).slice(0,32)){const color='#'+rgb.toString(16).padStart(6,'0'),b=document.createElement('button');b.style.background=color;b.title=color;b.setAttribute('aria-label','Color '+color);b.onclick=()=>q('pixelColor').value=color;q('pixelPalette').append(b);}q('pixelZoom').value=canvas.width>1600?'.25':canvas.width>800?'.5':'2';}dialog.showModal();layout();}catch(e){toast(e.message);}};
q('pixelDownload').onclick=()=>canvas.toBlob(blob=>{const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=(parent?.title||'atlas')+'-pixeles.png';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);},'image/png');
q('pixelSave').onclick=async()=>{finish();if(!dirty)return toast('Pinta la mejora antes de guardar.');const b=q('pixelSave');b.disabled=true;try{const base64=canvas.toDataURL('image/png').split(',')[1];const a=await api('/api/import',{base64,title:parent.title,version:'Edición manual de píxeles',parent_id:parent.id,provenance:'Pintor de píxeles de Atlas · copia exacta con trazos manuales'});if(!assets.some(x=>x.id===a.id))assets.push(a);dirty=false;dialog.close();await choose(a.id);toast('Nueva versión de píxeles guardada en Atlas.');}catch(e){toast(e.message);}finally{b.disabled=false;}};
})();
