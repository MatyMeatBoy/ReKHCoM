'use strict';
/* One settings model, shared with kh_com.ini and PortWebOption. */
const optionWord=(en,es)=>interfaceLanguage==='es'?es:en;
const optionLabel=(en,es)=>{const s=document.createElement('span');s.dataset.en=en;s.dataset.es=es;s.textContent=optionWord(en,es);return s;};
const optionSpecs=[
  /* as the PC launcher (decompui): the mode's presets apply on the next start; HD mode needs the HD mod, not on the web */
  ['graphics','Picture','Imagen','graphics_mode','Graphics mode','Modo gráfico',0,[[0,'Choose on start','Escoger al empezar'],[2,'GBA enhanced','GBA mejorado'],[3,'GBA classic','GBA clásico']],'startup'],
  ['graphics','Picture','Imagen','gba_border','GBA classic border','Borde del GBA clásico',0,[[0,'None','Ninguno'],[1,'Design 1','Diseño 1'],[2,'Design 2','Diseño 2'],[3,'Design 3','Diseño 3']]],
  ['graphics','Picture','Imagen','view_mode','View','Vista',4,[[0,'Original GBA','GBA original'],[1,'Widescreen','Panorámica'],[2,'Enhanced','Ampliada'],[3,'Maximum','Máxima'],[4,'Auto','Automática']]],
  ['graphics','Picture','Imagen','zoom','Pixel zoom','Zoom de píxeles',8,[[4,'×2','×2'],[6,'×3','×3'],[8,'×4','×4'],[10,'×5','×5'],[12,'×6','×6'],[14,'×7','×7'],[16,'×8','×8']]],
  ['graphics','Picture','Imagen','aspect','Aspect ratio','Proporción',0,[[0,'Fill window','Toda la ventana'],[1,'Up to 16:9','Hasta 16:9'],[2,'Up to 21:9','Hasta 21:9'],[3,'21:9','21:9'],[4,'32:9','32:9'],[5,'4:3','4:3'],[6,'GBA 3:2','GBA 3:2']]],
  ['graphics','Picture','Imagen','scale_mode','Scaling','Escalado',0,[[0,'Integer','Entero'],[1,'Fit','Ajustado'],[2,'Stretch','Estirado']]],
  ['graphics','Picture','Imagen','filter','Filter','Filtro',0,[[0,'Nearest pixels','Píxeles nítidos'],[1,'Sharp','Nítido adaptable'],[2,'Linear','Suave']]],
  ['graphics','Text & interface','Texto e interfaz','hd_text','HD text','Texto HD',0,'check'],
  ['graphics','Text & interface','Texto e interfaz','hud_size','HUD size','Tamaño del HUD',1,[[0,'Small','Pequeño'],[1,'Normal','Normal'],[2,'Large','Grande']]],
  ['graphics','Text & interface','Texto e interfaz','card_hud_raise','Raise card hand','Elevar mano de cartas',8,[0,16]],
  ['graphics','Text & interface','Texto e interfaz','scanlines','Scanlines','Líneas de pantalla',0,[0,100]],
  ['graphics','Text & interface','Texto e interfaz','color_correction','GBA colours','Colores GBA',0,'check'],
  ['graphics','Text & interface','Texto e interfaz','show_fps','Show FPS','Mostrar FPS',0,'check'],
  ['sound','Volume','Volumen','master_volume','Master','General',100,[0,100]],
  ['sound','Volume','Volumen','music_volume','Music','Música',100,[0,100]],
  ['sound','Volume','Volumen','sfx_volume','Sound effects','Efectos',100,[0,100]],
  ['sound','Extras','Extras','undub','Japanese voices','Voces japonesas',0,'check','startup'],
  ['sound','Extras','Extras','smooth_audio','Smooth audio (experimental)','Audio suave (experimental)',0,'check'],
  ['improvements','Battle','Combate','dodge_button','Dedicated dodge button','Botón para esquivar',1,'check'],
  ['improvements','Battle','Combate','stock_button','Dedicated stock button','Botón para acumular',1,'check'],
  ['improvements','Battle','Combate','invert_card_scroll','Invert card scrolling','Invertir selección de cartas',0,'check'],
  ['improvements','Battle','Combate','cheat_one_hit','Defeat enemies in one hit','Derrotar enemigos de un golpe',0,'check'],
  ['improvements','Battle','Combate','cheat_infinite_hp','Infinite HP','HP infinito',0,'check'],
  ['improvements','Extras','Extras','cheat_unlock_riku','Riku on title screen','Riku en el título',0,'check'],
  ['improvements','Extras','Extras','save_bank','Save bank','Banco de partidas',1,[[1,'1','1'],[2,'2','2'],[3,'3','3'],[4,'4','4'],[5,'5','5']],'startup'],
  ['improvements','Experimental','Experimental','scene_framing','Scene framing','Encuadre de escenas',0,[[0,'Adjusted','Ajuste'],[1,'Extended','Extendido'],[2,'Original shot','Plano original']]],
  ['improvements','Experimental','Experimental','experimental_battle_zoom','Battle zoom (Auto view)','Zoom en combate (vista Auto)',0,[[0,'Same as main zoom','Igual al zoom principal'],[4,'×2','×2'],[6,'×3','×3'],[8,'×4','×4'],[10,'×5','×5'],[12,'×6','×6'],[14,'×7','×7'],[16,'×8','×8']]],
  ['improvements','Experimental','Experimental','experimental_save_point_slot_select','Choose slot at save points','Elegir ranura en cristales',0,'check'],
  ['improvements','Experimental','Experimental','experimental_moogle_notice','Moogle points notice','Aviso de puntos moguri',0,'check'],
  ['improvements','Experimental','Experimental','experimental_unstock','Return stocked cards','Devolver cartas acumuladas',0,'check'],
  ['improvements','Experimental','Experimental','ps2_mode','Re:CoM mode','Modo Re:CoM',0,'check','startup'],
  ['improvements','Experimental','Experimental','ps2_difficulty','Re:CoM difficulty','Dificultad Re:CoM',0,[[0,'Standard','Normal'],[1,'Beginner','Principiante'],[2,'Proud','Experto']],'startup'],
  ['improvements','Experimental','Experimental','ps2_fast_text','Fast dialogue','Diálogos rápidos',0,'check'],
  ['improvements','Experimental','Experimental','experimental_hide_touch_controls','Hide touch panels','Ocultar paneles táctiles',0,'check'],
  ['controls','Touch screen','Pantalla táctil','touch_theme','Touch button theme','Tema de botones',0,[[0,'Clear','Clear'],[1,'Dark','Dark'],[2,'Color','Color']]],
  ['controls','Controller','Mando','pad_layout','Force Nintendo layout','Forzar diseño Nintendo',0,[[0,'Detect the pad','Detectar el mando'],[2,'On','Sí']]],
  ['sound','Extras','Extras','game_language','Game language (Europe)','Idioma del juego (Europa)',0,[[0,'Ask on start','Preguntar al empezar'],[1,'English','English'],[2,'Français','Français'],[3,'Deutsch','Deutsch'],[4,'Italiano','Italiano'],[5,'Español','Español']],'startup']
];
const bindingSpecs=[['a','A',120,0],['b','B',122,1],['l','L',97,9],['r','R',115,10],['start','START',13,6],['select','SELECT',8,4],['up','↑',1073741906,11],['down','↓',1073741905,12],['left','←',1073741904,13],['right','→',1073741903,14],['stock','Stock / Acumular',100,3],['dodge','Dodge / Esquivar',99,2],['unstock','Return / Devolver',118,8]];
const padChoices=[[0,'A'],[1,'B'],[2,'X'],[3,'Y'],[9,'LB'],[10,'RB'],[108,'LT'],[110,'RT'],[6,'START'],[4,'BACK'],[7,'LS'],[8,'RS'],[11,'D-pad ↑'],[12,'D-pad ↓'],[13,'D-pad ←'],[14,'D-pad →'],[101,'LS ←'],[100,'LS →'],[103,'LS ↑'],[102,'LS ↓'],[105,'RS ←'],[104,'RS →'],[107,'RS ↑'],[106,'RS ↓'],[-1,'—']];
const preferences=Object.fromEntries(optionSpecs.map(s=>[s[3],s[6]]));
for(const [key,,kb,pad]of bindingSpecs){preferences['kb_'+key]=kb;preferences['pad_'+key]=pad;}
const dirtyPreferences=new Set();
try{const saved=JSON.parse(localStorage.getItem('rekhcom-options')||'{}');for(const k in preferences)if(Number.isInteger(saved[k]))preferences[k]=saved[k];}catch{}
let baseIni='',listeningKey=null;
const optionInputs=new Map();
function preferenceNotice(en,es){$('preferences-status').textContent=optionWord(en,es);}
function keyName(v){return ReKHWeb.keyboardName(v)||optionWord('Unknown key','Tecla desconocida');}
function rememberPreferences(){try{localStorage.setItem('rekhcom-options',JSON.stringify(preferences));}catch{preferenceNotice('Browser settings storage is unavailable.','No se pueden conservar las opciones del navegador.');}}
function mergeIni(){let current='';try{if(core)current=core.FS.readFile('/saves/kh_com.ini',{encoding:'utf8'});}catch{}
  /* the file as it is now: the game and the language selector write it too */
  let text=current||baseIni||'config_version=11\n';for(const[k,v]of Object.entries(preferences)){const line=new RegExp('^'+k+'=.*$','m');text=line.test(text)?text.replace(line,k+'='+v):text+'\n'+k+'='+v;}return text+'\n';}
function changePreference(key,value){
  preferences[key]=Number(value);dirtyPreferences.add(key);rememberPreferences();
  if(started){
    if(!core.ccall||!core._PortWebOption){preferenceNotice('Reload the updated engine to change options.','Recarga el motor actualizado para cambiar opciones.');return;}
    preferences[key]=core.ccall('PortWebOption','number',['string','number','number'],[key,value,1]);
    if(key==='experimental_hide_touch_controls'){$('touch').checked=!value;core._PortWebTouchEnable(value?0:1);}
    core._PortWebResize($('canvas').width,$('canvas').height);store.request();
  }else if(core){core.FS.writeFile('/saves/kh_com.ini',mergeIni());store.request();}
  preferenceNotice('Settings saved.','Opciones guardadas.');
}
function addOption(spec){
  const[tab,enGroup,esGroup,key,en,es,initial,kind,locked]=spec;
  const groupId=tab+'-'+enGroup.replace(/\W/g,'');let group=$(groupId);
  if(!group){group=document.createElement('div');group.className='option-group';group.id=groupId;const heading=document.createElement('h2');heading.append(optionLabel(enGroup,esGroup));group.append(heading);$('options-'+tab).append(group);}
  const label=document.createElement('label');label.className='option-row';label.append(optionLabel(en,es));let input;
  if(Array.isArray(kind)&&Array.isArray(kind[0])){input=document.createElement('select');for(const[v,a,b]of kind){const option=document.createElement('option');option.value=v;option.dataset.en=a;option.dataset.es=b;option.textContent=optionWord(a,b);input.append(option);}input.value=preferences[key];}
  else{input=document.createElement('input');input.type=kind==='check'?'checkbox':'range';if(input.type==='range'){input.min=kind[0];input.max=kind[1];input.value=preferences[key];}else input.checked=!!preferences[key];}
  input.id='option-'+key;input.dataset.key=key;if(locked)input.dataset.startup='1';optionInputs.set(key,input);
  if(input.type==='range'){const wrap=document.createElement('div');wrap.className='volume-control';const out=document.createElement('output');out.htmlFor=input.id;out.textContent=input.value;wrap.append(input,out);label.append(wrap);input.addEventListener('input',()=>out.textContent=input.value);}else label.append(input);
  input.addEventListener('change',()=>changePreference(key,input.type==='checkbox'?+input.checked:Number(input.value)));group.append(label);
  if(locked){const note=document.createElement('p');note.className='option-note';note.append(optionLabel('Choose before starting the game.','Se elige antes de iniciar el juego.'));group.append(note);}
}
for(const spec of optionSpecs)addOption(spec);
for(const[key,label,kb,pad]of bindingSpecs){
  const row=document.createElement('div');row.className='option-row binding-row';const name=document.createElement('span');name.textContent=label;const button=document.createElement('button');button.textContent=keyName(preferences['kb_'+key]);button.setAttribute('aria-label',label+' keyboard');const select=document.createElement('select');select.setAttribute('aria-label',label+' gamepad');
  for(const[v,text]of padChoices){const o=document.createElement('option');o.value=v;o.textContent=text;select.append(o);}select.value=preferences['pad_'+key];
  button.addEventListener('click',()=>{if(listeningKey)listeningKey.button.classList.remove('listening');listeningKey={key,button};button.classList.add('listening');button.textContent='…';preferenceNotice('Press a key; Escape cancels, Delete clears.','Pulsa una tecla; Escape cancela, Suprimir borra.');});
  select.addEventListener('change',()=>{changePreference('pad_'+key,Number(select.value));drawWebController(key);});
  row.addEventListener('focusin',()=>drawWebController(key));row.addEventListener('pointerenter',()=>drawWebController(key));
  optionInputs.set('kb_'+key,button);optionInputs.set('pad_'+key,select);row.append(name,button,select);$('options-controls').append(row);
}
document.addEventListener('keydown',event=>{
  if(!listeningKey)return;event.preventDefault();event.stopImmediatePropagation();
  if(event.key!=='Escape'){const value=event.key==='Delete'?0:ReKHWeb.keyboardBinding(event);if(value===null){preferenceNotice('This key cannot be assigned; choose another.','Esta tecla no se puede asignar; elige otra.');return;}changePreference('kb_'+listeningKey.key,value);}
  else preferenceNotice('Key binding unchanged.','Asignación de tecla sin cambios.');
  listeningKey.button.textContent=keyName(preferences['kb_'+listeningKey.key]);listeningKey.button.classList.remove('listening');listeningKey=null;
},true);
window.ReKHOptions={paused:false,
  load(ini){baseIni=ini||'';for(const match of baseIni.matchAll(/^([a-z0-9_]+)=(-?\d+)$/gm))if(match[1]in preferences&&!dirtyPreferences.has(match[1]))preferences[match[1]]=Number(match[2]);this.refresh();},
  refresh(){for(const[k,input]of optionInputs){if(k.startsWith('kb_'))input.textContent=keyName(preferences[k]);else if(input.type==='checkbox')input.checked=!!preferences[k];else input.value=preferences[k];const out=input.parentElement.querySelector('output');if(out)out.textContent=input.value;input.disabled=started&&!!input.dataset.startup;}drawWebController();},
  beforeStart(){if(core){core.FS.writeFile('/saves/kh_com.ini',mergeIni());store.request();}},
  translate(){document.querySelectorAll('[data-en]').forEach(el=>{el.textContent=el.dataset[interfaceLanguage]||el.dataset.en;});}
};
function selectOptionsTab(id){
  for(const tab of document.querySelectorAll('[role=tab]')){const selected=tab.dataset.panel===id;tab.setAttribute('aria-selected',String(selected));tab.tabIndex=selected?0:-1;$(tab.getAttribute('aria-controls')).hidden=!selected;}
  ReKHOptions.paused=id!=='game';clearInput();visibility();scheduleResize();
  if(id==='achievements')window.ReKHAchievements?.render();
  if(id==='controls')drawWebController();
}
const tabs=Array.from(document.querySelectorAll('[role=tab]'));
tabs.forEach((tab,index)=>{tab.addEventListener('click',()=>selectOptionsTab(tab.dataset.panel));tab.addEventListener('keydown',event=>{let n;if(event.key==='ArrowRight')n=(index+1)%tabs.length;if(event.key==='ArrowLeft')n=(index+tabs.length-1)%tabs.length;if(event.key==='Home')n=0;if(event.key==='End')n=tabs.length-1;if(n!==undefined){event.preventDefault();tabs[n].focus();selectOptionsTab(tabs[n].dataset.panel);}});});
$('interface-language').addEventListener('change',()=>ReKHOptions.translate());
ReKHOptions.translate();
/* Same approved sprite, integer grid and masks as port_control_diagram.inc.
   This is an interactive UI layer; the cartridge-style source PNG stays intact. */
let controllerFocus='a',controllerPixels=null;
const controllerRegions=new Int16Array(104*79).fill(-1);
function drawWebController(key=controllerFocus){
  controllerFocus=key;const canvas=$('controller-diagram');if(!canvas)return;
  const scale=Math.max(1,Math.min(4,Math.floor(canvas.parentElement.getBoundingClientRect().width/104)));
  canvas.style.width=(104*scale)+'px';canvas.style.height=(79*scale)+'px';
  const binding=preferences['pad_'+key],selection=ReKHWeb.controllerBinding(binding);
  const action=bindingSpecs.find(s=>s[0]===key)?.[1]||key,name=padChoices.find(p=>p[0]===binding)?.[1]||'—';
  $('controller-binding').textContent=action+' → '+name;
  canvas.setAttribute('aria-label','8BitDo Ultimate 2C · '+action+' → '+name);
  if(!controllerPixels)return;
  const ctx=canvas.getContext('2d'),image=ctx.createImageData(104,79);image.data.set(controllerPixels.data);
  function lit(x,y){
    if(x<0||x>=104||y<0||y>=79||selection.region<0||controllerRegions[y*104+x]!==selection.region)return false;
    if(selection.region!==7&&selection.region!==8)return true;
    const dx=x-(selection.region===7?20:68),dy=y-(selection.region===7?27:44),d=selection.directions;
    return !!((d&1&&dy<=0)||(d&2&&dy>=0)||(d&4&&dx<=0)||(d&8&&dx>=0));
  }
  for(let y=7;y<79;y++)for(let x=0;x<104;x++){
    const p=(y*104+x)*4;if(!image.data[p+3])continue;
    if(lit(x,y)){
      const r=controllerRegions[y*104+x],edge=x===0||x===103||y===7||y===78||
        controllerRegions[y*104+x-1]!==r||controllerRegions[y*104+x+1]!==r||controllerRegions[(y-1)*104+x]!==r||controllerRegions[(y+1)*104+x]!==r;
      if(edge){image.data[p]=156;image.data[p+1]=16;image.data[p+2]=0;}
      else if(image.data[p]+image.data[p+1]+image.data[p+2]>180){image.data[p]=(image.data[p]+255)>>1;image.data[p+1]=(image.data[p+1]+158)>>1;image.data[p+2]=(image.data[p+2]+52)>>1;}
    }else if(lit(x-1,y)||lit(x+1,y)||lit(x,y-1)||lit(x,y+1)){image.data[p]=255;image.data[p+1]=230;image.data[p+2]=142;}
  }
  ctx.putImageData(image,0,0);
  const letters={L:0x4927,R:0x6bad,T:0x7492,B:0x6bae};
  for(const[x,region,label]of [[4,15,'LT'],[22,9,'LB'],[65,10,'RB'],[83,16,'RT']]){
    const on=selection.region===region;ctx.fillStyle=on?'#9c1000':'#43343c';ctx.fillRect(x,0,17,7);ctx.fillStyle=on?'#cb563d':'#78626b';ctx.fillRect(x+1,1,15,5);ctx.fillStyle='#fff2cd';
    for(let c=0;c<label.length;c++)for(let yy=0;yy<5;yy++)for(let xx=0;xx<3;xx++)if(letters[label[c]]&(1<<(14-yy*3-xx)))ctx.fillRect(x+5+c*4+xx,1+yy,1,1);
  }
}
const controllerImage=new Image();
controllerImage.onload=()=>{
  const canvas=$('controller-diagram'),ctx=canvas.getContext('2d');ctx.clearRect(0,0,104,79);ctx.drawImage(controllerImage,0,7);controllerPixels=ctx.getImageData(0,0,104,79);
  for(const[cx,cy,r,id]of [[20,27,7,7],[68,44,7,8],[83,20,4,3],[75,28,4,2],[90,28,4,1],[83,36,4,0],[37,18,3,4],[67,18,3,6],[52,18,4,5]])
    for(let y=cy-r;y<=cy+r;y++)for(let x=cx-r;x<=cx+r;x++)if((x-cx)**2+(y-cy)**2<=r*r+1)controllerRegions[y*104+x]=id;
  for(let y=35;y<=54;y++)for(let x=28;x<=46;x++){
    if(x>=34&&x<=39)controllerRegions[y*104+x]=y<42?11:y>47?12:-1;
    else if(y>=42&&y<=47)controllerRegions[y*104+x]=x<34?13:14;
  }
  for(let y=7;y<=10;y++)for(let x=16;x<=30;x++){controllerRegions[y*104+x]=9;controllerRegions[y*104+103-x]=10;}
  drawWebController();
};
controllerImage.src='controller.png';
window.addEventListener('resize',()=>drawWebController());
