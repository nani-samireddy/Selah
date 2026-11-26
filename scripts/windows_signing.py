"""Authenticode signing for the Windows release (certificate-store keys)."""
import os
from pathlib import Path
import re
import shutil
import subprocess
from urllib.parse import urlparse


def configuration():
    """Validate inputs before building; never read/export private key material."""
    thumbprint = re.sub(r"\s", "", os.environ.get("SELAH_SIGN_CERT_SHA1", ""))
    timestamp = os.environ.get("SELAH_SIGN_TIMESTAMP_URL", "")
    parsed = urlparse(timestamp)
    if not re.fullmatch(r"[0-9a-fA-F]{40}", thumbprint):
        raise RuntimeError("Set SELAH_SIGN_CERT_SHA1 to the signing certificate's 40-digit thumbprint")
    if parsed.scheme not in ("https", "http") or not parsed.hostname or parsed.username or parsed.password:
        raise RuntimeError("Set SELAH_SIGN_TIMESTAMP_URL to your provider's RFC3161 HTTP(S) endpoint")
    tool = shutil.which("signtool.exe") or shutil.which("signtool")
    if not tool:
        raise RuntimeError("SignTool missing: use a Windows SDK developer shell with signtool on PATH")
    return tool, thumbprint.upper(), timestamp


def sign_executable(executable, config):
    tool, thumbprint, timestamp = config
    executable = Path(executable).resolve(strict=True)
    # /sha1 selects the certificate; /fd and /td still use SHA-256.
    command = [tool, "sign", "/sha1", thumbprint, "/s", "My",
               "/fd", "SHA256", "/tr", timestamp, "/td", "SHA256", "/d", "Selah"]
    if os.environ.get("SELAH_SIGN_MACHINE_STORE") == "1":
        command.append("/sm")
    subprocess.run([*command, str(executable)], check=True)
    # Warnings are nonzero too: do not publish an ambiguously verified build.
    verify_executable(executable, config)
    return {"signing": "authenticode", "signedFiles": ["bin/Selah.exe"],
            "signingCertificateSHA1": thumbprint, "timestampURL": timestamp,
            "signatureVerified": True, "archiveSigned": False}


def verify_executable(executable, config):
    subprocess.run([config[0], "verify", "/pa", "/all", "/v", "/tw", str(executable)], check=True)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("Authenticode signing requires Windows")
    sign_executable(args.executable, configuration())
