"""Package identity/configuration only: no SDK, native build, ROM or device."""
import importlib.util
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('build_android',ROOT/'port/tools/build_android.py')
build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
ANDROID='{http://schemas.android.com/apk/res/android}'
class PackageTests(unittest.TestCase):
 def test_beta_archive_names_preserve_test_isolation(self):
  identities=[build.package_identity('v2.1'),build.package_identity('v2.1',smoke=True),
              build.package_identity('v2.1',test_build=True,test_version='2.1-nightly.20261027.1')]
  self.assertEqual([build.package_filename(i) for i in identities],
                   ['ReKHCoM-v2.1-android-beta.apk','ReKHCoM-v2.1-android-beta-smoke.apk',
                    'ReKHCoM-v2.1-nightly.20261027.1-android-beta-optiport.apk'])
 def test_isolated_playable_identity(self):
  identity=build.package_identity('v2.0',test_build=True,test_version='2.1-nightly.20261027.1')
  self.assertEqual(identity['application_id'],'com.rekhcom.android.optiport')
  self.assertEqual(identity['suffix'],'-optiport')
  self.assertEqual(identity['version_name'],'2.1-nightly.20261027.1-optiport')
  self.assertEqual(identity['version_code'],2010001)
  original=(ROOT/'port/android/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
  text=build.playable_test_manifest(original,identity)
  self.assertNotIn('SDL_ENV.',text)
  app=ET.fromstring(text).find('application')
  self.assertEqual(app.attrib[ANDROID+'label'],'ReKHCoM OptiPort 2.1-nightly.20261027.1')
  activities={a.attrib[ANDROID+'name']:a for a in app.findall('activity')}
  self.assertEqual(activities['com.rekhcom.android.GameActivity'].attrib[ANDROID+'exported'],'false')
  self.assertIn('com.rekhcom.android.LauncherActivity',activities)
 def test_stable_and_smoke_preserved(self):
  stable=build.package_identity('v2.0')
  smoke=build.package_identity('v2.0',smoke=True)
  self.assertEqual(stable['application_id'],'com.rekhcom.android')
  self.assertEqual(stable['suffix'],'')
  self.assertEqual(stable['version_name'],'2.0-android-beta')
  self.assertEqual(smoke['application_id'],'com.rekhcom.android.smoke')
  self.assertEqual(smoke['suffix'],'-smoke')
 def test_official_upgrade_order_preserves_published_codes(self):
  self.assertEqual(build.package_identity('v2.1')['version_code'],2010099)
  self.assertEqual(build.package_identity('v2.2-pre1')['version_code'],2020099)
  releases=['v2.2-pre1','v2.2-pre2','v2.2-pre98','v2.2','v2.2.1-pre1',
            'v2.2.1-pre98','v2.2.1','v2.3-pre1','v2.3','v3.0-pre1','v3.0']
  codes=[build.package_identity(r)['version_code'] for r in releases]
  self.assertTrue(all(a<b for a,b in zip(codes,codes[1:])),codes)
  for release in releases:
   self.assertEqual(build.package_identity(release)['application_id'],'com.rekhcom.android')
  self.assertEqual(build.package_identity('v2.2-pre2')['version_code'],102020002)
 def test_official_code_capacity_and_android_bound(self):
  self.assertLessEqual(build.package_identity('v1999.99.99')['version_code'],2100000000)
  for release in ['v2.2-pre0','v2.2-pre99','v2.2-pre100','v2.100','v2.2.100',
                  'v2000.0','v2.2-garbage','v2.2.1-pre2-extra']:
   with self.subTest(release=release),self.assertRaises(ValueError):
    build.package_identity(release)
 def test_reject_conflicting_or_unsafe_identity(self):
  for args in [dict(test_build=True,smoke=True),dict(test_version='2.1'),dict(test_build=True),
               dict(test_build=True,test_version="2.1-nightly.'bad"),dict(test_build=True,test_version='2.1-nightly.20261027.100')]:
   with self.subTest(args=args),self.assertRaises(ValueError):build.package_identity('v2.0',**args)
if __name__=='__main__':unittest.main()
