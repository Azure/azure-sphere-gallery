#!/usr/bin/env python3

from pathlib import Path
import subprocess
import os

exclude_paths=[
    "MT3620_M4_Sample_Code",
    "AzureSphereDevX/examples",
    "threadx"
]

BUILD_OK = "Build OK"
BUILD_FAILED = "Build failed"
GENERATE_FAILED = "Generate failed"
MISSING_PRESETS = "Missing CMakePresets.json"
NOT_SPHERE = "Not an Azure Sphere project (Consider adding to excludes)"

class Messages:
    def __init__(self):
        self.messages = {}

    def add(self, category, instance, detail = None):
        if not category in self.messages.keys():
            self.messages[category] = []
        self.messages[category] += [ (instance, detail) ]

    def categories(self):
        return self.messages.keys()
    
    def instances(self, category):
        if category not in self.messages.keys():
            return []
        return self.messages[category]

class Log:
    def __init__(self): pass

    def _log(self, level, message):
        print(f"::{level}::{message}")

    def error(self, message):
        self._log("error", message)

    def notice(self, message):
        self._log("notice", message)

def should_exclude(path, exclude_paths):
    for e in exclude_paths:
        if e in path:
            return True
    return False

def code(str, lang=None):
    lang = lang or ""
    return f"```{lang}\n{str}\n```\n";

def indent(str, amount=1):
    return "\n".join( ("   " * amount) + line for line in str.split('\n') )


def build(cmakelists, log, messages):
    preset = "ARM-Release"
    generate_args = [ "cmake", f"--preset {preset}", str(cmakelists) ]
    generate = subprocess.run( generate_args, capture_output=True)
    if generate.returncode != 0:
        log.error(f"{p.parent}: Generate failed")
        detail = indent(code(generate.stderr.decode("utf-8")))
        messages.add(GENERATE_FAILED, p.parent, detail)
        return False

    folder = p.parent.joinpath("out").joinpath(preset)
    build_args = [ "cmake", "--build", folder]
    build = subprocess.run(build_args, capture_output=True)
    if build.returncode != 0:
        log.error(f"{p.parent}: Build failed")
        detail = indent(code(generate.stderr.decode("utf-8")))
        messages.add(BUILD_FAILED, p.parent, detail)
        return False

    messages.add(BUILD_OK, p.parent)
    return True

cmakelists = ( path 
               for path in list(Path(".").rglob("CMakeLists.txt"))
               if not should_exclude(str(path), exclude_paths) )

log = Log()
messages = Messages()

success = True

for p in cmakelists:
    azsphere_project = False
    with open(p,"r") as cmakelists:
        for line in cmakelists:
            if "azsphere_configure_tools" in line:
                azsphere_project = True
    if not azsphere_project:
        messages.add(NOT_SPHERE, p)
        continue

    folder = p.parent
    if not folder.joinpath("CMakePresets.json").exists():
        messages.add(MISSING_PRESETS, folder)
        continue
    success = success and build(p, log, messages)

with open(os.environ.get("GITHUB_STEP_SUMMARY", "summary.md"), "w") as summary:
    for category in (BUILD_OK, BUILD_FAILED, GENERATE_FAILED, MISSING_PRESETS, NOT_SPHERE):
        summary.write(f"# {category}\n")
        for (message, detail) in sorted(messages.instances(category), key=lambda i: i[0]):
            summary.write(f" * **{message}**\n")
            if detail:
                summary.write(f"{detail}\n")

    summary.write("# Projects considered")
    for p in cmakelists:
        summary.write(f" * {p}")

exit(0 if success else -1)