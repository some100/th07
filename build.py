import argparse
import hashlib
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path, PureWindowsPath
from typing import Dict, Iterator, List, Tuple
from zipfile import ZipFile

import requests

SCRIPT_PATH = Path(os.path.abspath(__file__))
SCRIPT_DIR = Path(os.path.dirname(SCRIPT_PATH))

os.chdir(SCRIPT_DIR)

SRC_DIR = SCRIPT_DIR / "src"
BUILD_DIR = SCRIPT_DIR / "build"
OBJ_DIR = BUILD_DIR / "objects"
MSVC_PATH = SCRIPT_DIR / "thirdparty" / "msvc"
DX8_PATH = SCRIPT_DIR / "thirdparty" / "dx8"
EXE_PATH = SCRIPT_DIR / "th07.exe"
BUILD_PATH = BUILD_DIR / "th07.exe"

VS_PATH = MSVC_PATH / "PROGRAM FILES" / "MICROSOFT VISUAL STUDIO .NET"
VC_PATH = VS_PATH / "VC7"
CL_PATH = VC_PATH / "BIN" / "CL.EXE"
LINK_PATH = VC_PATH / "BIN" / "LINK.EXE"
RC_PATH = VC_PATH / "BIN" / "RC.EXE"

DX8_URL = "https://archive.org/download/dx8sdk/dx8sdk.exe"
MSVC_URL = "https://archive.org/download/en_vs.net_pro_full/en_vs.net_pro_full.exe"
HACKERY_URL = "https://gist.githubusercontent.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d/raw/822536a26025f0df8763f1112d89bb1514f6209c/hackery.cpp"
DX8_SIZE = 144441256
MSVC_SIZE = 1706945024

SOURCES: List[Path] = list(
    map(
        lambda x: Path(x),
        [
            "AnmVm.cpp",
            "AsciiManager.cpp",
            "Stage.cpp",
            "BombData.cpp",
            "EclManager.cpp",
            "EnemyEclInstr.cpp",
            "EffectManager.cpp",
            "Ending.cpp",
            "EnemyManager.cpp",
            "BulletManager.cpp",
            "Gui.cpp",
            "GameManager.cpp",
            "Chain.cpp",
            "Controller.cpp",
            "FileSystem.cpp",
            "GameErrorContext.cpp",
            "Rng.cpp",
            "utils.cpp",
            "TextHelper.cpp",
            "ItemManager.cpp",
            "main.cpp",
            "GameWindow.cpp",
            "MidiOutput.cpp",
            "Supervisor.cpp",
            "MusicRoom.cpp",
            "Player.cpp",
            "ReplayManager.cpp",
            "ResultScreen.cpp",
            "ScreenEffect.cpp",
            "SoundPlayer.cpp",
            "AnmManager.cpp",
            "MainMenu.cpp",
            "dsutil.cpp",
            "pbg4/Pbg4File.cpp",
            "pbg4/Lzss.cpp",
            "pbg4/Pbg4Archive.cpp",
        ],
    )
)

parser = argparse.ArgumentParser()
_ = parser.add_argument("--no-icon", action="store_true")
_ = parser.add_argument("--no-matching", action="store_true")
args = parser.parse_args()


# Oh My God Bruh
def fixup_msiextract(path: Path):

    # msiextract makes absolutely no attempt at resolving the mappings and renaming.
    # this is done to extract the product name from the files (the first part of it)
    # since we dont really care about the rest.
    # do files first to avoid issues with renaming directories before files
    for file in path.rglob("*"):
        if not file.is_dir():
            name = file.name
            if ":" in name and "|" in name:
                name = name.split("|")[0]
                name = name.split(":")[0]
                _ = file.rename(file.with_name(name))

    dirs = [p for p in path.rglob("*") if p.is_dir()]

    dirs.sort(key=lambda p: len(p.parts), reverse=True)
    for dir in dirs:
        name = dir.name
        if ":" in name and "|" in name:
            name = name.split("|")[0]
            name = name.split(":")[0]
            name = name.upper()
            if not dir.with_name(name).exists():
                _ = dir.rename(dir.with_name(name))
        # this is done specifically for the folders in platformsdk.
        # just get the part out that looks like a directory name
        elif name.startswith(".:"):
            name = name.split(":")[1]
            name = name.upper()
            if not dir.with_name(name).exists():
                _ = dir.rename(dir.with_name(name))
            else:
                _ = shutil.copytree(dir, dir.with_name(name), dirs_exist_ok=True)


def download(url: str, dest_path: Path):
    response = requests.get(url, stream=True)
    total = int(response.headers.get("content-length", 0))
    if response.status_code == 200:
        with open(dest_path, "wb") as file:
            downloaded = 0
            chunk_iter: Iterator[bytes] = response.iter_content(chunk_size=1024 * 1024)
            for data in chunk_iter:
                downloaded += file.write(data)
                percent = (downloaded / total) * 100
                print(f"\rDownloading {url} ... {percent:.2f}%", end="")
    else:
        raise Exception(f"Failed to download {url}")


def download_dx8():
    if not DX8_PATH.exists():
        os.makedirs(DX8_PATH)
    if (DX8_PATH / "include").exists():
        return
    archive_path = DX8_PATH / "dx8sdk.exe"
    if not os.path.exists(archive_path) or archive_path.stat().st_size != DX8_SIZE:
        download(DX8_URL, archive_path)

    # these happen to be valid zips, too
    with ZipFile(archive_path, "r") as zip:
        zip.extractall(DX8_PATH)


# provides pragma var_order
def install_hackery(env: Dict[str, str]):
    c1xx_path = VC_PATH / "BIN" / "C1XX.DLL"
    orig_path = VC_PATH / "BIN" / "C1XXOrig.dll"

    if orig_path.exists() and c1xx_path.exists():
        return
    os.chdir(MSVC_PATH)
    download(HACKERY_URL, MSVC_PATH / "hackery.cpp")
    if not orig_path.exists():
        _ = shutil.copy(c1xx_path, orig_path)
    _ = run_program(
        str(CL_PATH),
        "/LD",
        conv_path(MSVC_PATH / "hackery.cpp"),
        "/link",
        f"/OUT:{MSVC_PATH / 'C1XX.DLL'}",
        env=env,
    )
    _ = (MSVC_PATH / "C1XX.DLL").replace(c1xx_path)


# this is awful
def download_msvc():
    if not MSVC_PATH.exists():
        os.makedirs(MSVC_PATH)
    if (VC_PATH / "BIN" / "CL.EXE").exists():
        return
    archive_path = MSVC_PATH / "en_vs.net_pro_full.exe"
    if not os.path.exists(archive_path) or archive_path.stat().st_size != MSVC_SIZE:
        download(MSVC_URL, archive_path)
    with ZipFile(archive_path, "r") as zip:
        _ = zip.extractall(MSVC_PATH)
    if sys.platform == "win32":
        _ = subprocess.check_call(
            [
                "msiexec",
                "/a",
                str(MSVC_PATH / "VS_SETUP.MSI"),
                "/qb",
                f'TARGETDIR="{MSVC_PATH}"',
            ]
        )
    else:
        _ = subprocess.check_call(
            ["msiextract", "-C", MSVC_PATH, MSVC_PATH / "VS_SETUP.MSI"]
        )
        fixup_msiextract(MSVC_PATH)
        # lets just assume you're on a case sensitive filesystem
        _ = shutil.copytree(
            MSVC_PATH / "Program Files", MSVC_PATH / "PROGRAM FILES", dirs_exist_ok=True
        )
        _ = shutil.rmtree(MSVC_PATH / "Program Files")

    # we dont really need anything that isnt already inside of program files
    for file in MSVC_PATH.iterdir():
        if (
            file.name.upper() == "PROGRAM FILES"
            or file.name == "en_vs.net_pro_full.exe"
        ):
            continue
        if file.is_file():
            os.remove(file)
        elif file.is_dir():
            shutil.rmtree(file)
    # cl dll dependencies
    _ = (VS_PATH / "COMMON7" / "IDE" / "MSPDB70.DLL").rename(
        VC_PATH / "BIN" / "MSPDB70.DLL"
    )
    _ = (VS_PATH / "COMMON7" / "IDE" / "MSOBJ10.DLL").rename(
        VC_PATH / "BIN" / "MSOBJ10.DLL"
    )


def run_program(name: str, *args: str, env: Dict[str, str]):
    if sys.platform == "win32":
        cmd = [name] + list(args)
    else:
        env["LANG"] = "ja_JP.UTF-8"
        env["WINEDEBUG"] = "fixme-all"
        cmd = ["wine", name] + list(args)
    return subprocess.check_call(cmd, env=env)


def run_program_capture(name: str, *args: str, env: Dict[str, str]):
    if sys.platform == "win32":
        cmd = [name] + list(args)
    else:
        env = env.copy()
        env["LANG"] = "ja_JP.UTF-8"
        env["WINEDEBUG"] = "fixme-all"
        cmd = ["wine", name] + list(args)

    result = subprocess.run(
        cmd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    return result.returncode, result.stdout


def conv_path(path: Path) -> str:
    """Convert a Unix path to a Windows path, if needed."""
    if sys.platform == "win32":
        return str(PureWindowsPath(path))

    return "Z:" + str(path.resolve()).replace("/", "\\")


def extract_icon(path: Path) -> Path:
    """Extract the icon from a PE file located at path"""
    ico_path = BUILD_DIR / path.with_suffix(".ico").name

    # icoextract doesn't have an actual scripting API, so it has to be shelled out to instead.
    # should be fine since you're running this through uv anyways which puts the venv in PATH
    cmd = ["icoextract", str(path), ico_path]
    _ = subprocess.check_call(cmd)
    return ico_path


def compile_resources(path: Path) -> Path:
    rc_path = BUILD_DIR / "resources.rc"
    with open(rc_path, "w") as f:
        _ = f.write(f'1 ICON "{path.name}"\n')
    res_path = rc_path.with_suffix(".res")
    _ = run_program(
        str(RC_PATH), f"/fo{conv_path(res_path)}", conv_path(rc_path), env=env
    )
    return res_path


def compile(src: Path) -> Tuple[int, str]:
    if not needs_compile(src):
        return 0, ""

    obj = OBJ_DIR / src.with_suffix(".obj")

    returncode, output = run_program_capture(
        str(CL_PATH),
        "/c",
        conv_path(SRC_DIR / src),
        "/Fo" + conv_path(obj),
        "/Fd" + conv_path(obj.with_suffix(".pdb")),
        *cflags,
        env=env,
    )

    return returncode, output

def needs_compile(src: Path) -> bool:
    obj = OBJ_DIR / src.with_suffix(".obj")
    if not (OBJ_DIR / src).parent.exists():
        os.makedirs((OBJ_DIR / src).parent, exist_ok=True)
    hpp = SRC_DIR / src.with_suffix(".hpp")

    return not (
        os.path.exists(obj)
        and os.path.getmtime(obj) >= os.path.getmtime(SRC_DIR / src)
        and (not hpp.exists() or os.path.getmtime(obj) >= os.path.getmtime(hpp))
        and os.path.getmtime(obj) >= os.path.getmtime(SCRIPT_PATH)
    )

def handle_compile_result(result: Tuple[int, str]):
    if result[1] != "":
        print(result[1], flush=True)
    if result[0] != 0:
        sys.exit(1)

env = os.environ.copy()

download_dx8()
download_msvc()
env["INCLUDE"] = (
    conv_path(VC_PATH / "INCLUDE")
    + ";"
    + conv_path(VC_PATH / "PLATFORMSDK" / "COMMON" / "Include")
)
env["LIB"] = (
    conv_path(VC_PATH / "LIB")
    + ";"
    + conv_path(VC_PATH / "PLATFORMSDK" / "COMMON" / "lib")
)
install_hackery(env)

out = BUILD_DIR / "th07.exe"

cflags = [
    "/nologo",
    "/W3",
    "/MT",
    "/Od",
    "/Ob1",
    "/Oi",
    "/EHsc",
    "/Gr",
    "/GL",
    "/Gy",
    "/Gf",
    "/Zi",
    "/DNDEBUG",
    f"-I{conv_path(DX8_PATH / 'include')}",
]

if args.no_matching:
  cflags.append("/DNON_MATCHING")

lflags = [
    f"-LIBPATH:{conv_path(DX8_PATH / 'lib')}",
    "/LTCG",
    "/INCREMENTAL:NO",
    "/MAP",
    "/OPT:REF",
    "/OPT:NOICF",
    "/DEBUG",
]

libs = [
    "dinput8.lib",
    "dsound.lib",
    "d3d8.lib",
    "d3dx8.lib",
    "dxguid.lib",
    "gdi32.lib",
    "user32.lib",
    "winmm.lib",
    "ole32.lib",
]

objects: List[str] = list(
    map(lambda x: conv_path(OBJ_DIR / x.with_suffix(".obj")), SOURCES)
)

pending = [src for src in SOURCES if needs_compile(src)]

os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

with ThreadPoolExecutor() as executor:
    futures = {executor.submit(compile, src): src for src in pending}

    for future in as_completed(futures):
        src = futures[future]
        handle_compile_result(future.result())

if not args.no_icon:
    objects.append(conv_path(compile_resources(extract_icon(EXE_PATH))))

_ = run_program(
    str(LINK_PATH),
    *objects,
    *lflags,
    *libs,
    f"/OUT:{out}",
    env=env,
)
