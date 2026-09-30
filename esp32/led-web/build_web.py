# Builds the React app in web/ into data/ before PlatformIO packs data/ into the filesystem image.
import os
import subprocess

Import("env")

WEB_DIR = os.path.join(env.subst("$PROJECT_DIR"), "web")


def run(cmd):
    # Capture output and re-print it safely: Vite prints characters (e.g. a check mark)
    # that crash PlatformIO on consoles with a non-UTF-8 codepage.
    out = subprocess.run(cmd, cwd=WEB_DIR, capture_output=True)
    text = (out.stdout + out.stderr).decode("utf-8", "replace")
    print(text.encode("ascii", "replace").decode("ascii"))
    if out.returncode:
        raise SystemExit("web build failed: " + " ".join(cmd))


def build_web(*args, **kwargs):
    npm = "npm.cmd" if os.name == "nt" else "npm"
    if not os.path.isdir(os.path.join(WEB_DIR, "node_modules")):
        run([npm, "install"])
    run([npm, "run", "build"])


env.AddPreAction("$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin", build_web)
