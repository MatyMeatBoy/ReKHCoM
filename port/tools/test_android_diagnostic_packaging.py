"""Regression for Android process identifiers aapt does not reject."""
import unittest
from build_android_diagnostics import validate_private_process

class ProcessNames(unittest.TestCase):
    def test_existing_and_optional_private_processes(self):
        for value in (':game', ':startup_profile', ':another.valid_process'):
            validate_private_process(value)
    def test_bad_private_names_fail_even_in_optimized_python(self):
        for value in (':startup-profile', ':', ':123', '../bad', 'global'):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_private_process(value)

if __name__ == '__main__':
    unittest.main()
