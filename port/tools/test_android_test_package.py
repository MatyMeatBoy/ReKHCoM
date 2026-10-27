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
 def test_reject_conflicting_or_unsafe_identity(self):
  for args in [dict(test_build=True,smoke=True),dict(test_version='2.1'),dict(test_build=True),
               dict(test_build=True,test_version="2.1-nightly.'bad"),dict(test_build=True,test_version='2.1-nightly.20261027.100')]:
   with self.subTest(args=args),self.assertRaises(ValueError):build.package_identity('v2.0',**args)
if __name__=='__main__':unittest.main()
