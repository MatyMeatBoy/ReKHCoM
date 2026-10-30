"""Source-only inventory regressions; no ROM, renderer or live Atlas writes."""
import copy
from pathlib import Path
import tempfile
import unittest

from art_review.scenario_inventory import camera_segments, catalog_links, inventory, render_contexts


class ScenarioInventoryTests(unittest.TestCase):
    def test_camera_deadlines_use_gba_u16_and_next_target(self):
        rows = [
            ['-65535','100','200','0','EVENT_CHARA_NONE','0','0','NULL'],
            ['-65116','300','400','0','EVENT_CHARA_NONE','CAMERA_MODE_APPROACH','150','NULL'],
            ['-55537','500','600','0','EVENT_CHARA_NONE','CAMERA_KEYFRAME_FLAG_END','50','NULL'],
        ]
        segments = camera_segments(rows, 600)
        self.assertEqual([(s['start'],s['until']) for s in segments], [(0,1),(1,420),(420,600)])
        self.assertEqual(segments[1]['settled_at'],151)
        self.assertEqual(segments[2]['settled_at'],470)
        self.assertTrue(segments[2]['end_sentinel'])
        self.assertNotIn(9999, [f for s in segments for f in s['capture_frames']])

    def test_invalid_timeline_is_visible_instead_of_silently_omitted(self):
        with self.assertRaisesRegex(ValueError,'deadline'):
            camera_segments([['not_a_number','0','0','0','0','0','0','NULL']],20)

    def test_unreached_deadlines_beyond_event_end_do_not_create_segments(self):
        rows=[['20','0','0','0','0','0','0','NULL'],
              ['90','0','0','0','0','0','10','NULL'],
              ['9999','0','0','0','0','CAMERA_KEYFRAME_FLAG_END','10','NULL']]
        self.assertEqual([(r['start'],r['until']) for r in camera_segments(rows,40)],[(0,20),(20,40)])

    def test_catalog_links_keep_historical_status_without_approving_other_views(self):
        assets=[
            dict(id='accepted',title='Escena 004 · integrada',status='approved',revision_info={}),
            dict(id='pending',title='Otro título',status='pending',revision_info={'catalog_key':'scene-130'}),
            dict(id='unrelated',title='Naipe130 / versión004',status='approved',revision_info={}),
        ]
        original=copy.deepcopy(assets)
        self.assertEqual([a['id'] for a in catalog_links(4,assets)],['accepted'])
        self.assertEqual([a['id'] for a in catalog_links(130,assets)],['pending'])
        self.assertEqual(catalog_links(131,assets),[])
        self.assertEqual(assets,original)

    def test_catalog_link_follows_renamed_versions_without_replacing_approval(self):
        assets=[dict(id='first',title='Escena130',status='approved'),
                dict(id='new',title='Retocado',status='pending',parent_id='first')]
        links=catalog_links(130,assets)
        self.assertEqual([a['id'] for a in links],['first','new'])
        self.assertEqual([a['title'] for a in links],['Escena130','Retocado'])
        self.assertEqual([a['historical_status'] for a in links],['approved','pending'])
        assets[0]['parent_id']='new'  # malformed historical cycle must not hang
        self.assertEqual(len(catalog_links(130,assets)),2)
        assets[0].pop('parent_id')
        assets.extend([dict(id='other',title='Escena004',status='approved'),
                       dict(title='Missing identifier',status='pending')])
        self.assertEqual([a['id'] for a in catalog_links(130,assets)],['first','new'])

    def test_existing_coliseum_scene_titles_are_not_new_missing_backgrounds(self):
        assets=[dict(id=str(n),title=f'Coliseo {n} · revisión',status='approved') for n in (121,122,124)]
        for number in (121,122,124):
            self.assertEqual([a['id'] for a in catalog_links(number,assets)],[str(number)])

    def test_explicit_shared_reference_ids_are_typed_and_do_not_imply_approval(self):
        asset=dict(id='shared',title='Fondo compartido',status='pending',revision_info={'scene_ids':[75,86]})
        for number in (75,86):
            self.assertEqual([a['id'] for a in catalog_links(number,[asset])],['shared'])
            self.assertEqual(catalog_links(number,[asset])[0]['historical_status'],'pending')
        self.assertEqual(catalog_links(76,[asset]),[])
        asset['revision_info']['scene_ids']=[True,'75']
        self.assertEqual(catalog_links(1,[asset]),[])
        self.assertEqual(catalog_links(75,[asset]),[])

    def test_field_and_battle_contexts_keep_sources_and_null_story_alternatives(self):
        with tempfile.TemporaryDirectory(prefix='scenario-contexts-') as tmp:
            root=Path(tmp)
            (root/'include/menu').mkdir(parents=True);(root/'src/debug').mkdir(parents=True)
            (root/'include/menu/world_types.h').write_text('enum WorldId { WORLD_ONE=1,WORLD_TWO=2,WORLD_NONE=0xFF };')
            (root/'src/debug/mode_chkbtl.c').write_text('''
static ChkBtlEntry sChkBtlEntries[2] = {{WORLD_ONE,0,BATTLE_NORMAL,NULL},{WORLD_TWO,2,BATTLE_BOSS,NULL}};
static ChkBtlWorld sChkBtlWorlds[2] = {{BATTLE_STAGE_ONE,NULL},{BATTLE_STAGE_TWO,NULL}};
''')
            contexts=render_contexts(root)
            self.assertEqual([w['symbol'] for w in contexts['field_worlds']],['WORLD_ONE','WORLD_TWO'])
            self.assertEqual([b['battle'] for b in contexts['battle_entries']],['BATTLE_NORMAL','BATTLE_BOSS'])
            self.assertEqual(contexts['battle_stages'],['BATTLE_STAGE_ONE','BATTLE_STAGE_TWO'])
            self.assertEqual(contexts['runtime_coverage'],'UNVERIFIED')

    def test_inventory_covers_events_outside_curated_menu_and_retains_shared_bg(self):
        with tempfile.TemporaryDirectory(prefix='scenario-inventory-') as tmp:
            root=Path(tmp)
            for name in ['include/evt','src/evt/events']:(root/name).mkdir(parents=True)
            (root/'include/evt/event_ids.h').write_text('enum EventId { EVENT_000_ONE, EVENT_001_TWO, EVENT_002_NULL, EVENT_130_LIBRARY=130 };')
            pointers=['&gEventBg000','&gEventBg000','NULL']+['NULL']*127+['&gEventBg130']
            (root/'src/evt/event_index_data.c').write_text('const EventBackgroundDef* gEventBackgroundDefs[] = {'+','.join(pointers)+'};')
            for i in [0,1,2,130]:
                (root/f'src/evt/events/{i:03}_test.inc').write_text(f'''
static const EventCameraKeyframe sEvent{i:03}Camera[1] = {{
 {{ -55537, 100, 200, 0, EVENT_CHARA_NONE, CAMERA_KEYFRAME_FLAG_END, 0, NULL }},
}};
const EventSequenceDef gEvent{i:03} = {{0,NULL,sEvent{i:03}Camera,NULL,NULL,NULL,60,FALSE}};
''')
            report=inventory(root,[])
            self.assertEqual([r['event'] for r in report['events']],[0,1,2,130])
            self.assertEqual(report['backgrounds']['gEventBg000']['events'],[0,1])
            self.assertIsNone(report['events'][2]['background'])
            library=report['events'][3]
            self.assertEqual(library['background'],'gEventBg130')
            self.assertEqual(library['runtime_coverage'],'UNVERIFIED')
            self.assertEqual(report['issues'],[])
            (root/'src/evt/events/130_test.inc').unlink()
            report=inventory(root,[])
            self.assertEqual(len(report['events']),4)
            self.assertTrue(any(e['event']==130 and 'script' in e['reason'] for e in report['issues']))

    def test_region_specific_enum_holes_do_not_shift_background_identity(self):
        with tempfile.TemporaryDirectory(prefix='scenario-region-') as tmp:
            root=Path(tmp)
            for name in ['include/evt','src/evt/events']:(root/name).mkdir(parents=True)
            (root/'include/evt/event_ids.h').write_text('''enum EventId {
 EVENT_000_START,
#ifndef VERSION_EU
 EVENT_001_DUMMY,
#endif
 EVENT_002_LIBRARY, EVENT_COUNT };''')
            (root/'src/evt/event_index_data.c').write_text('''const EventBackgroundDef* gEventBackgroundDefs[] = {
 &gEventBg000,
#ifndef VERSION_EU
 NULL,
#endif
 &gEventBg002 };''')
            for i in range(3):
                (root/f'src/evt/events/{i:03}_test.inc').write_text(f'''
static const EventCameraKeyframe sEvent{i:03}Camera[] = {{
 {{9999,0,0,0,0,CAMERA_KEYFRAME_FLAG_END,0,NULL}},
}};
const EventSequenceDef gEvent{i:03} = {{0,NULL,sEvent{i:03}Camera,NULL,NULL,NULL,30,FALSE}};''')
            report=inventory(root,[])
            library=next(r for r in report['events'] if r['event']==2)
            self.assertEqual(library['background'],'gEventBg002')
            self.assertEqual({r['region']:r['runtime_event'] for r in library['regional_variants']},dict(eu=1,us=2,jp=2))
            self.assertEqual(report['issues'],[])


if __name__=='__main__':unittest.main()
