'use strict';
/* The official set (achievement-data.js), as the game earns it offline:
   /saves/achievements_official_eu.dat ("KHOFF1", then "<id> <time>" per unlock).
   Badges: achievement-badges.png, one atlas (10 per row in set order, 64x64). */
function readOfficialAchievements(text){
  const lines=String(text).trim().split(/\r?\n/),earned=new Map();
  if(lines[0]!=='KHOFF1')return earned;
  for(const line of lines.slice(1)){const m=/^(\d+)\s+(\d+)$/.exec(line.trim());if(m)earned.set(Number(m[1]),Number(m[2]));}
  return earned;
}
function achievementProgress(){
  try{return readOfficialAchievements(core.FS.readFile('/saves/achievements_official_eu.dat',{encoding:'utf8'}));}
  catch{return new Map();}
}
function renderAchievements(){
  const set=window.ReKHOfficialAchievements||[],earned=achievementProgress(),grid=$('achievement-grid');grid.replaceChildren();
  const got=set.filter(a=>earned.has(a[0])),points=got.reduce((n,a)=>n+a[1],0),total=set.reduce((n,a)=>n+a[1],0);
  $('achievement-progress').textContent=got.length+' / '+set.length+' · '+points+' / '+total+' '+optionWord('points','puntos');
  $('achievement-meter').max=set.length;$('achievement-meter').value=got.length;
  const query=$('achievement-search').value.trim().toLocaleLowerCase(),filter=$('achievement-filter').value;
  set.forEach(([id,value,texts],index)=>{
    const [title,description]=texts[interfaceLanguage==='es'?4:0];
    const unlocked=earned.has(id);
    if(filter==='unlocked'&&!unlocked||filter==='locked'&&unlocked||query&&!(title+' '+description).toLocaleLowerCase().includes(query))return;
    const row=document.createElement('article');row.className='achievement-row'+(unlocked?' unlocked':'');
    const badge=document.createElement('span');badge.className='achievement-badge';badge.setAttribute('aria-hidden','true');
    badge.style.backgroundPosition=(-(index%10)*48)+'px '+(-Math.floor(index/10)*48)+'px';
    const text=document.createElement('div'),heading=document.createElement('h2'),detail=document.createElement('p'),state=document.createElement('small');
    heading.textContent=title;detail.textContent=description;
    state.textContent=value+' '+(value===1?optionWord('point','punto'):optionWord('points','puntos'))+' · '+(unlocked?optionWord('Unlocked','Conseguido'):optionWord('Locked','No conseguido'));
    text.append(heading,detail,state);row.append(badge,text);grid.append(row);
  });
  if(!grid.children.length){const empty=document.createElement('p');empty.textContent=optionWord('No achievements match this filter.','No hay logros con ese filtro.');grid.append(empty);}
}
$('achievement-search').addEventListener('input',renderAchievements);
$('achievement-filter').addEventListener('change',renderAchievements);
$('tab-achievements').addEventListener('click',renderAchievements);
$('interface-language').addEventListener('change',renderAchievements);
window.ReKHAchievements={render:renderAchievements,read:readOfficialAchievements};
renderAchievements();
