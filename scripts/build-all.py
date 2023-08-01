#!/usr/bin/env python3

from pathlib import Path
import subprocess

exclude_paths=[
    "MT3620_M4_Sample_Code",
    "AzureSphereDevX/examples",
    "threadx"
]

def log(level, message):
    print(f"::{level}::{message}")

def error(message):
    log("error", message)

def notice(message):
    log("notice", message)

def should_exclude(path, exclude_paths):
    for e in exclude_paths:
        if e in path:
            return True
    return False

def build(cmakelists):
    preset = "ARM-Release"
    generate_args = [ "cmake", f"--preset {preset}", str(cmakelists) ]
    generate = subprocess.run( generate_args, capture_output=True)
    if generate.returncode == 0:
        folder = p.parent.joinpath("out").joinpath(preset)
        build_args = [ "cmake", "--build", folder]
        build = subprocess.run(build_args, capture_output=True)
        if build.returncode == 0:
            print(f"{p.parent}: Build OK")
        else:
            error(f"{p.parent}: Build failed: {build.stderr}")
    else:
        error(f"{p.parent}: Generate failed: {generate.stderr}")

cmakelists = ( path 
               for path in list(Path(".").rglob("CMakeLists.txt"))
               if not should_exclude(str(path), exclude_paths) )

for p in cmakelists:
    azsphere_project = False
    with open(p,"r") as cmakelists:
        for line in cmakelists:
            if "azsphere_configure_tools" in line:
                azsphere_project = True
    if not azsphere_project:
        notice(f"{p}: not an Azure Sphere project; skipping")
        continue

    folder = p.parent
    if not folder.joinpath("CMakePresets.json").exists():
        error(f"{folder}: missing CMakePresets.json; skipping")
        continue
    build(p)