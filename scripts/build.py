import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from download_deps import conv_path, download_dx8, download_msvc, install_hackery

SCRIPT_PATH = Path(os.path.abspath(__file__))
PROJ_DIR = Path(os.path.dirname(SCRIPT_PATH)).parent

os.chdir(PROJ_DIR)

SRC_DIR = PROJ_DIR / "src"
BUILD_DIR = PROJ_DIR / "build"
RESOURCE_DIR = PROJ_DIR / "resources"
CUSTOM_RESOURCE_DIR = PROJ_DIR / "resources" / "custom"
MSVC_PATH = PROJ_DIR / "thirdparty" / "msvc"
DX8_PATH = PROJ_DIR / "thirdparty" / "dx8"
EXE_PATH = RESOURCE_DIR / "th07.exe"
CUSTOM_EXE_PATH = RESOURCE_DIR / "custom.exe"

VS_PATH = MSVC_PATH / "Program Files" / "Microsoft Visual Studio .NET"
VC_PATH = VS_PATH / "Vc7"
CL_PATH = VC_PATH / "bin" / "cl.exe"
LINK_PATH = VC_PATH / "bin" / "link.exe"
RC_PATH = VC_PATH / "bin" / "rc.exe"

TH07_SOURCES = [
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
]

TH07_SMALL = [
    "GameManager.cpp",
    "Gui.cpp",
    "MainMenu.cpp",
    "MusicRoom.cpp",
    "ResultScreen.cpp",
    "Supervisor.cpp",
    "TextHelper.cpp",
]

TH07_SMALL_NON_INTRINSIC = [
    "MainMenu.cpp",
    "ResultScreen.cpp",
    "TextHelper.cpp",
]

CUSTOM_SOURCES = [
    "init.cpp",
    "main.cpp",
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "--no-icon",
    action="store_true",
    help="build without requiring an icon from the original executable",
)
parser.add_argument(
    "--no-matching", action="store_true", help="build without attempting matching"
)
parser.add_argument(
    "--with-custom",
    action="store_true",
    help="use reccmp on custom (configuration tool) as well",
)
subparsers = parser.add_subparsers(dest="command")

parser_reccmp = subparsers.add_parser("reccmp", help="output reccmp output")
parser_stackcmp = subparsers.add_parser(
    "stackcmp", help="compare stack layout with stackcmp"
)
parser_datacmp = subparsers.add_parser("datacmp", help="compare globals with datacmp")
parser_roadmap = subparsers.add_parser(
    "roadmap", help="compare symbol locations with roadmap"
)

parser_reccmp.add_argument(
    "address",
    nargs="?",
    default=None,
    help="optional function address for displaying diff",
)
parser_reccmp.add_argument(
    "--init", action="store_true", help="initialize reccmp project"
)
parser_reccmp.add_argument("--svg", action="store_true", help="generate progress svg")
parser_stackcmp.add_argument(
    "address", help="function address for displaying stack layout"
)
args = parser.parse_args()


def extract_icon(path: Path, id: int, output: Path):
    cmd = ["icoextract", "-i", str(id), str(path), output]
    subprocess.check_call(cmd)


if not CUSTOM_EXE_PATH.exists():
    if args.with_custom:
        sys.exit("custom.exe should exist when building custom.exe")

if not EXE_PATH.exists():
    if args.command:
        sys.exit("th07.exe should exist when using any reccmp tools")
    elif not args.no_icon:
        sys.exit("th07.exe should exist when building with icon")

if not shutil.which("ninja"):
    sys.exit("ninja must be installed when building")

download_dx8(DX8_PATH)
download_msvc(MSVC_PATH, VS_PATH, VC_PATH)
install_hackery(CL_PATH, MSVC_PATH, VC_PATH)

INCLUDES = [
    VC_PATH / "include",
    VC_PATH / "PlatformSDK" / "include",
    DX8_PATH / "include",
    SRC_DIR / "th07",
    SRC_DIR / "custom",
    BUILD_DIR / "obj" / "custom",
]

LIB_PATHS = [
    VC_PATH / "lib",
    VC_PATH / "PlatformSDK" / "lib",
    DX8_PATH / "lib",
]

cflags_base = ["/nologo", "/W3", "/GF", "/Zi"] + [
    f'/I"{conv_path(p)}"' for p in INCLUDES
]

lflags_base = [f'/LIBPATH:"{conv_path(p)}"' for p in LIB_PATHS] + [
    "/INCREMENTAL:NO",
    "/MAP",
    "/DEBUG",
    "/OPT:REF",
    "/OPT:ICF",
]

libs_base = ["dinput8.lib", "user32.lib"]

cflags_th07 = cflags_base + [
    "/Ob1",
    "/MT",
    "/EHsc",
    "/Gr",
    "/GL",
    "/Gy",
    "/DNDEBUG",
    "/wd4060",
    "/wd4101",
    "/wd4244",
]

if args.no_matching:
    cflags_th07.append("/DNON_MATCHING")

lflags_th07 = lflags_base + ["/LTCG"]

libs_th07 = libs_base + [
    "dsound.lib",
    "d3d8.lib",
    "d3dx8.lib",
    "dxguid.lib",
    "gdi32.lib",
    "winmm.lib",
    "ole32.lib",
]

cflags_custom = cflags_base + ["/ML", "/O1", "/Ob0", "/GS"]
lflags_custom = lflags_base
libs_custom = libs_base

rcflags = [f'/i "{conv_path(p)}"' for p in INCLUDES]

os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

with open("build.ninja", "w") as f:
    f.write("ninja_required_version = 1.3\n\n")

    if sys.platform != "win32":
        wine_cmd = "env LANG=ja_JP.UTF-8 WINEDEBUG=fixme-all wine "
        f.write(
            f'cl = "{sys.executable}" ../scripts/cl_wrapper.py "{conv_path(CL_PATH)}"\n'
        )
    else:
        wine_cmd = ""
        f.write(f'cl = "{conv_path(CL_PATH)}"\n')

    f.write(f'link = {wine_cmd}"{conv_path(LINK_PATH)}"\n')
    f.write(f'rc = {wine_cmd}"{conv_path(RC_PATH)}"\n\n')

    f.write(f"cflags_th07 = {' '.join(cflags_th07)}\n")
    f.write(f"cflags_th07_normal = /Od /Oi $cflags_th07\n")
    f.write(f"cflags_th07_small = /Os /Oi $cflags_th07\n")
    f.write(f"cflags_th07_small_nonintrinsic = /Os $cflags_th07\n")
    f.write(f"cflags_custom = {' '.join(cflags_custom)}\n\n")

    f.write(f"lflags_th07 = {' '.join(lflags_th07)}\n")
    f.write(f"lflags_custom = {' '.join(lflags_custom)}\n\n")

    f.write(f"libs_th07 = {' '.join(libs_th07)}\n")
    f.write(f"libs_custom = {' '.join(libs_custom)}\n\n")

    f.write(f"rcflags = {' '.join(rcflags)}\n\n")

    f.write("rule cxx\n")
    f.write("  command = $cl /showIncludes $in_cflags /c $in /Fo$out /Fd$pdb\n")
    f.write("  description = cxx $out\n")
    f.write("  deps = msvc\n\n")

    f.write("rule rc\n")
    f.write("  command = $rc $rcflags /fo$out $in\n")
    f.write("  description = rc $out\n\n")

    f.write("rule link\n")
    f.write("  command = $link $in $in_lflags $in_libs /OUT:$out\n")
    f.write("  description = link $out\n\n")

    f.write("rule i18n\n")
    f.write("  command = python ../scripts/generate_i18n.py $in $out\n")
    f.write("  description = i18n $out\n\n")

    f.write("build ../src/th07/i18n.hpp: i18n ../resources/csv/i18n.csv\n")

    th07_objects = []
    for src in TH07_SOURCES:
        src_path = f"../src/th07/{src}"
        obj_path = f"obj/th07/{Path(src).with_suffix('.obj').as_posix()}"
        pdb_path = f"obj/th07/{Path(src).with_suffix('.pdb').as_posix()}"

        cflags_to_use = "$cflags_th07_normal"
        if src in TH07_SMALL_NON_INTRINSIC:
            cflags_to_use = "$cflags_th07_small_nonintrinsic"
        elif src in TH07_SMALL:
            cflags_to_use = "$cflags_th07_small"

        f.write(f"build {obj_path}: cxx {src_path} | ../src/th07/i18n.hpp\n")
        f.write(f"  in_cflags = {cflags_to_use}\n")
        f.write(f"  pdb = {pdb_path}\n")
        th07_objects.append(obj_path)
    f.write("\n")

    if not args.no_icon:
        icon_path = BUILD_DIR / EXE_PATH.with_suffix(".ico").name
        extract_icon(EXE_PATH, 105, icon_path)
        try:
            with open("resources.rc", "x") as g:
                g.write(f'105 ICON "{icon_path.name}"\n')
        except FileExistsError:
            pass

        f.write("build resources.res: rc resources.rc\n\n")
        th07_objects.append("resources.res")

    f.write(f"build th07.exe: link {' '.join(th07_objects)}\n")
    f.write(f"  in_lflags = $lflags_th07\n")
    f.write(f"  in_libs = $libs_th07\n")

    if args.with_custom:
        f.write("build ../src/custom/i18n.hpp: i18n ../resources/custom/csv/i18n.csv\n")

        custom_objects = []
        for src in CUSTOM_SOURCES:
            src_path = f"../src/custom/{src}"
            obj_path = f"obj/custom/{Path(src).with_suffix('.obj').as_posix()}"
            pdb_path = f"obj/custom/{Path(src).with_suffix('.pdb').as_posix()}"

            f.write(f"build {obj_path}: cxx {src_path} | ../src/custom/i18n.hpp\n")
            f.write("  in_cflags = $cflags_custom\n")
            f.write(f"  pdb = {pdb_path}\n")
            custom_objects.append(obj_path)
        f.write("\n")

        os.makedirs(BUILD_DIR / "obj" / "custom", exist_ok=True)

        icon107_path = BUILD_DIR / "obj" / "custom" / "ICON107_1.ico"
        icon108_path = BUILD_DIR / "obj" / "custom" / "ICON108_1.ico"
        extract_icon(CUSTOM_EXE_PATH, 107, icon107_path)
        extract_icon(CUSTOM_EXE_PATH, 108, icon108_path)

        f.write("build obj/custom/resource.res: rc ../src/custom/resource.rc\n")
        custom_objects.append("obj/custom/resource.res")

        f.write(f"build custom.exe: link {' '.join(custom_objects)}\n")
        f.write(f"  in_lflags = $lflags_custom\n")
        f.write(f"  in_libs = $libs_custom\n")

subprocess.check_call(["ninja"])

match args.command:
    case "reccmp":
        if args.init:
            os.chdir(PROJ_DIR)
            subprocess.check_call(
                ["reccmp-project", "detect", "--search-path", RESOURCE_DIR]
            )
            os.chdir(BUILD_DIR)
            subprocess.check_call(["reccmp-project", "detect", "--what", "recompiled"])
        elif args.svg:
            subprocess.check_call(
                [
                    "reccmp-reccmp",
                    "--target",
                    "TH07",
                    "--svg",
                    RESOURCE_DIR / "progress.svg",
                    "--svg-icon",
                    RESOURCE_DIR / "svgicon.png",
                    "--nolib",
                ]
            )
            if args.with_custom:
                subprocess.check_call(
                    [
                        "reccmp-reccmp",
                        "--target",
                        "CUSTOM",
                        "--svg",
                        CUSTOM_RESOURCE_DIR / "progress.svg",
                        "--svg-icon",
                        CUSTOM_RESOURCE_DIR / "svgicon.png",
                        "--nolib",
                    ]
                )
        elif args.address:
            target = "CUSTOM" if args.with_custom else "TH07"
            subprocess.check_call(
                [
                    "reccmp-reccmp",
                    "--target",
                    target,
                    "--html",
                    "index.html",
                    "--nolib",
                    "--verbose",
                    args.address,
                ]
            )
        else:
            subprocess.check_call(
                ["reccmp-reccmp", "--target", "TH07", "--html", "index.html", "--nolib"]
            )
            if args.with_custom:
                subprocess.check_call(
                    [
                        "reccmp-reccmp",
                        "--target",
                        "CUSTOM",
                        "--html",
                        "custom.html",
                        "--nolib",
                    ]
                )
    case "stackcmp":
        subprocess.call(["reccmp-stackcmp", "--target", "TH07", args.address])
    case "datacmp":
        subprocess.call(["reccmp-datacmp", "--target", "TH07"])
    case "roadmap":
        subprocess.call(["reccmp-roadmap", "--target", "TH07"])
    case _:
        pass
