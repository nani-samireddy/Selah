"""Portable checks of installer orchestration; native Windows acceptance is separate."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import windows_installer
import windows_signing


class WindowsPackagingChecks(unittest.TestCase):
    def test_metadata_rejects_preprocessor_injection_and_invalid_version(self):
        for version, publisher in [("0.1", "Name"), ("65536.0.0", "Name"),
                                   ("0.1.0", ""), ("0.1.0", 'Name"\n[Run]'),
                                   ("0.1.0", "{code:run}")]:
            with self.assertRaises(RuntimeError):
                windows_installer.validate_metadata(version, publisher)
        windows_installer.validate_metadata("0.1.0", "Developer Name")

    def test_missing_stage_is_not_compiled(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(windows_installer.subprocess, "run") as run:
            with self.assertRaises(RuntimeError):
                windows_installer.build(tmp, Path(tmp)/"out", "0.1.0", "Name", "iscc")
            run.assert_not_called()

    def test_signed_compilation_uses_inno_callback_and_final_verification(self):
        with tempfile.TemporaryDirectory(prefix="selah installer ") as tmp:
            stage, output = Path(tmp)/"stage", Path(tmp)/"output"
            (stage/"bin").mkdir(parents=True)
            (stage/"bin/Selah.exe").write_bytes(b"test fixture")
            def compile_fixture(command, check):
                self.assertTrue(check)
                self.assertIn("/DSignedRelease=1", command)
                callback = next(x for x in command if x.startswith("/SSelahRelease="))
                self.assertTrue(callback.endswith(" $f"))
                self.assertIn("windows_signing.py", callback)
                (output/"Selah-0.1.0-Windows-x64-Setup.exe").write_bytes(b"installer fixture")
            config = ("signtool", "AB"*20, "https://timestamp.example.test")
            with patch.object(windows_installer.subprocess, "run", side_effect=compile_fixture), \
                 patch.object(windows_signing, "verify_executable") as verify:
                result = windows_installer.build(stage, output, "0.1.0", "Name", "iscc", config)
                verify.assert_called_once_with(result, config)

    def test_signing_failure_aborts_before_verification(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp)/"Selah.exe"
            exe.write_bytes(b"fixture")
            with patch.object(windows_signing.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "signtool")) as run:
                with self.assertRaises(subprocess.CalledProcessError):
                    windows_signing.sign_executable(exe, ("signtool", "AB"*20, "https://timestamp.example.test"))
                self.assertEqual(run.call_count, 1)

    def test_signing_configuration_requires_certificate_and_timestamp(self):
        with patch.dict(os.environ, {}, clear=True):
            with self.assertRaises(RuntimeError):
                windows_signing.configuration()


if __name__ == "__main__":
    unittest.main()
