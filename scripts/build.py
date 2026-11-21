#!/usr/bin/env python3
"""Serialize Selah's configure/build operations in a shared checkout."""
import argparse
from contextlib import contextmanager
from pathlib import Path
import os
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


@contextmanager
def build_lock(directory):
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / ".church-deck-build.lock").open("a+b") as handle:
        if os.name == "nt":
            import msvcrt
            handle.seek(0)
            if handle.read(1) == b"":
                handle.write(b"0")
                handle.flush()
            while True:
                try:
                    handle.seek(0)
                    msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError:
                    print("Waiting for the current Selah build…", flush=True)
                    time.sleep(1)
        else:
            import fcntl
            try:
                fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                print("Waiting for the current Selah build…", flush=True)
                fcntl.flock(handle, fcntl.LOCK_EX)
        try:
            yield
        finally:
            if os.name == "nt":
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle, fcntl.LOCK_UN)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configure", action="store_true")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--jobs", type=int, default=int(os.environ.get("CHURCHDECK_BUILD_JOBS", "4")))
    parser.add_argument("--target", action="append", default=[])
    parser.add_argument("--wait-for-pid", type=int, help=argparse.SUPPRESS)
    args = parser.parse_args()
    with build_lock(args.build_dir):
        # Adopt a build started before this wrapper was installed without interrupting it.
        if args.wait_for_pid:
            print("Waiting for the existing build to finish…", flush=True)
            while True:
                try:
                    os.kill(args.wait_for_pid, 0)
                except ProcessLookupError:
                    break
                time.sleep(1)
        if args.configure:
            command = ["cmake", "-S", str(ROOT), "-B", str(args.build_dir)]
            if shutil.which("ninja"):
                command += ["-G", "Ninja"]
            if shutil.which("brew"):
                prefix = subprocess.check_output(["brew", "--prefix"], text=True).strip()
                command += [f"-DCMAKE_PREFIX_PATH={prefix}", f"-DQt6_DIR={prefix}/lib/cmake/Qt6"]
            subprocess.run(command, check=True)
        command = ["cmake", "--build", str(args.build_dir), "--parallel", str(max(1, args.jobs))]
        if args.target:
            command += ["--target", *args.target]
        subprocess.run(command, check=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
    except KeyboardInterrupt:
        raise SystemExit(130)
