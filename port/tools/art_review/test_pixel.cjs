/* Offline browser regression. Run with Node + Playwright available on NODE_PATH:
   node port/tools/art_review/test_pan.cjs
   Uses mocked HTTP responses; never opens the real review store. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {chromium} = require('playwright');
const web = path.join(__dirname, 'web');
const fixture = {
  id:'pan-test', title:'Pan regression fixture', version:'test', width:1200, height:900,
  image_url:'/fixture.svg', status:'pending', general:'', revision:1, doodles:[],
  provenance:'Offline fixture', annotations:[{id:'cloud-1',x:.1,y:.1,w:.2,h:.2,
    comment:'Preserve this cloud',category:'general',priority:'normal',expected:'',lesson:''}],
};

(async()=>{
  const browser = await chromium.launch({headless:true});
  try {
    const page = await browser.newPage({viewport:{width:1440,height:950}});
    const errors=[], writes=[],imports=[];let child=null;
    page.on('pageerror',e=>errors.push(e.message));
    await page.route('**/*',async route=>{
      const request=route.request(),pathname=new URL(request.url()).pathname;
      if(request.method()!=='GET')writes.push(pathname);
      if(pathname==='/api/import'){const data=request.postDataJSON();imports.push(data);child={...fixture,id:'pixel-child',parent_id:fixture.id,version:data.version,annotations:[],doodles:[],general:''};return route.fulfill({json:child});}
      if(pathname==='/api/state')return route.fulfill({json:{assets:child?[fixture,child]:[fixture],approvals:[],queue:[],app_version:'test'}});
      if(pathname==='/api/history')return route.fulfill({json:{events:[{at:'2026-10-24T12:00:00Z',review:fixture}]}});
      if(pathname==='/fixture.svg')return route.fulfill({contentType:'image/svg+xml',body:'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="900"><rect width="1200" height="900" fill="tan"/></svg>'});
      const file=pathname==='/'?'index.html':pathname.slice(1);
      if(['index.html','app.js','style.css'].includes(file))return route.fulfill({path:path.join(web,file)});
      return route.abort();
    });
    await page.goto('http://art-review.test/');
    await page.waitForFunction(()=>document.getElementById('title').textContent==='Pan regression fixture');

    const original=await page.evaluate(()=>JSON.stringify(current));
    await page.locator('#pixelPaint').click();
    await page.locator('#pixelZoom').selectOption('8');
    await page.locator('#pixelColor').fill('#ff0000');
    const box=await page.locator('#pixelCanvas').boundingBox();
    const rgb=()=>page.locator('#pixelCanvas').evaluate(c=>Array.from(c.getContext('2d').getImageData(2,2,2,1).data));
    await page.mouse.click(box.x+20,box.y+20);
    assert.deepEqual(await rgb(),[255,0,0,255,210,180,140,255],'one exact pixel; neighbour unchanged');
    await page.locator('#pixelUndo').click();
    assert.deepEqual(await rgb(),[210,180,140,255,210,180,140,255],'undo restores original bytes');
    await page.mouse.click(box.x+20,box.y+20);
    const download=page.waitForEvent('download');await page.locator('#pixelDownload').click();
    await (await download).saveAs(path.resolve('build/atlas-pixel-smoke.png'));
    await page.locator('#pixelSave').click();
    await page.waitForFunction(()=>current?.id==='pixel-child');
    assert.equal(imports.length,1);assert.equal(imports[0].parent_id,fixture.id);
    assert.equal(imports[0].title,fixture.title);assert.ok(imports[0].base64.length);
    assert.deepEqual(writes,['/api/import']);
    assert.equal(await page.evaluate(id=>JSON.stringify(assets.find(a=>a.id===id)),fixture.id),original);
    assert.deepEqual(errors,[]);
    console.log('PASS: native pixel, unchanged neighbour, undo, PNG download, child import, original intact');
  }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
