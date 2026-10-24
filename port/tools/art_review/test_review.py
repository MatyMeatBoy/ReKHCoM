import hashlib
import io
import json
import tempfile
import threading
import unittest
import urllib.request
import urllib.error
from http.server import ThreadingHTTPServer
from pathlib import Path
from PIL import Image
from server import Store, handler


class ReviewTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.store = Store(self.temp.name)
        raw = io.BytesIO()
        Image.new('RGB', (100, 80), '#224433').save(raw, 'PNG')
        self.a = self.store.add(raw.getvalue(), 'Prueba <script>', 'commit-test')

    def tearDown(self):
        self.temp.cleanup()

    def note(self):
        return dict(id='nube-uno', x=.1, y=.25, w=.3, h=.5, comment='Reparar unión, conservar luz.',
                    category='seam', priority='high', expected='Sin corte', lesson='Respetar plano')

    def test_persistence_batch_crop_hash_and_learning(self):
        self.a['annotations'] = [self.note(), dict(self.note(),id='nube-dos',x=.6,w=.2)]
        self.a['general'] = 'Criterio humano: ñ y acentos'
        self.a = self.store.save(self.a)
        restored = Store(self.temp.name).get(self.a['id'])
        self.assertEqual(restored['annotations'][0]['comment'], self.note()['comment'])
        result = self.store.submit([self.a['id']], {self.a['id']:self.a['revision']})
        folder = Path(result['path']).parent
        payload = json.loads((folder/'comentarios.json').read_text(encoding='utf-8'))
        asset = payload['assets'][0]
        self.assertEqual(hashlib.sha256((folder/asset['snapshot']).read_bytes()).hexdigest(),asset['sha256'])
        self.assertEqual(asset['annotations'][0]['pixels_xyxy_exclusive'], [10,20,40,60])
        with Image.open(folder/asset['annotations'][0]['crop']) as crop:
            self.assertEqual(crop.size, (30,40))
        cases = [json.loads(line) for line in (folder/'aprendizaje.jsonl').read_text(encoding='utf-8').splitlines()]
        self.assertEqual(len(cases),3)
        self.assertEqual(cases[0]['kind'],'version_verdict')
        self.assertIsNone(cases[1]['subsequent_human_verdict'])
        self.assertIn('ñ y acentos',Path(result['path']).read_text(encoding='utf-8'))
        self.assertTrue((folder/'REVISION_PARA_CODEX.md').exists())
        self.assertTrue((folder/'aprobaciones_build.json').exists())
        self.assertEqual(self.store.get(self.a['id'])['submitted_revision'],1)
        # Deleted comments remain in the event history; immutable old exports stay unchanged.
        before=Path(result['path']).read_bytes()
        self.a['annotations']=[];self.store.save(self.a)
        self.assertEqual(before,Path(result['path']).read_bytes())
        with self.store.db() as db:
            events=db.execute("SELECT data FROM events WHERE kind='human_review_saved'").fetchall()
        self.assertEqual(len(json.loads(events[0][0])['annotations']),2)

    def test_conflicts_invalid_bounds_empty_notes(self):
        stale=dict(self.a)
        self.store.save(self.a)
        with self.assertRaisesRegex(ValueError,'otra pestaña'):self.store.save(stale)
        self.a=self.store.get(self.a['id'])
        self.a['annotations']=[dict(self.note(),x=.99)]
        with self.assertRaises(ValueError):self.store.save(self.a)
        self.a['annotations']=[dict(self.note(),comment=' ')]
        self.a=self.store.save(self.a)
        with self.assertRaisesRegex(ValueError,'necesita'):self.store.submit([self.a['id']])
        self.a['annotations']=[dict(self.note(),w=float('nan'))]
        with self.assertRaises(ValueError):self.store.save(self.a)

    def test_http_origin_and_traversal(self):
        server=ThreadingHTTPServer(('127.0.0.1',0),handler(self.store))
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        url='http://127.0.0.1:'+str(server.server_port)
        try:
            state=json.load(urllib.request.urlopen(url+'/api/state'))
            self.assertEqual(state['assets'][0]['id'],self.a['id'])
            request=urllib.request.Request(url+'/api/save',data=json.dumps(self.a).encode(),headers={'X-Art-Review':'local','Origin':'https://example.com'})
            with self.assertRaises(urllib.error.HTTPError) as error:urllib.request.urlopen(request)
            self.assertEqual(error.exception.code,403)
            with self.assertRaises(urllib.error.HTTPError) as error:urllib.request.urlopen(url+'/images/../../server.py')
            self.assertEqual(error.exception.code,404)
        finally:
            server.shutdown();server.server_close();thread.join()

    def test_new_version_has_parent_and_own_verdict(self):
        self.a['status']='honors'
        self.a=self.store.save(self.a)
        new=self.store.add((Path(self.temp.name)/'images'/(self.a['id']+'.png')).read_bytes(), 'Prueba', 'v2', parent_id=self.a['id'])
        self.assertEqual(new['parent_id'],self.a['id'])
        self.assertEqual(new['status'],'pending')
        self.assertEqual(new['annotations'],[])
        registry=self.store.approvals()[0]
        self.assertEqual(registry['build_version'],self.a['id'])
        new['status']='changes_requested';self.store.save(new)
        self.assertEqual(self.store.approvals()[0]['build_version'],self.a['id'])
        self.assertEqual(len(self.store.history(self.a['id'])),2)
        new=self.store.get(new['id']);new['status']='honors';self.store.save(new)
        self.assertEqual(self.store.approvals()[0]['build_version'],new['id'])


if __name__=='__main__':unittest.main()
