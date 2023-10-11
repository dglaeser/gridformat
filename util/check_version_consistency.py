# SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import sys
import subprocess
from os.path import join, abspath, dirname

def _run_and_capture(cmd: str) -> str:
    return subprocess.run(cmd.split(), check=True, capture_output=True, text=True).stdout


def _is_release_version_tag(tag: str) -> bool:
    if not tag.startswith("v"):
        return False
    tag = tag[1:]
    try:
        major, minor, patch = tag.split(".")
        _, _, _ = int(major), int(minor), int(patch)
        return True
    except:
        return False


def _get_current_release_version_tag() -> str:
    tag_sha = _run_and_capture(f"git rev-list --tags --max-count=1").strip("\n")
    tag_name = _run_and_capture(f"git describe {tag_sha}").strip("\n")
    if not _is_release_version_tag(tag_name):
        raise RuntimeError(f"Latest tag is not a release tag")
    return tag_name


def _get_cmake_version() -> str:
    with open(join(dirname(abspath(__file__)), "../CMakeLists.txt")) as cmake_file:
        return cmake_file.read().split("\nproject(")[1].split("VERSION ")[1].split(")")[0]


def _get_cff_version() -> str:
    with open(join(dirname(abspath(__file__)), "../CITATION.cff")) as cff_file:
        return cff_file.read().split("\nversion: ")[1].split("\n")[0]


if __name__ == "__main__":
    tag_version = _get_current_release_version_tag()[1:]
    cff_version = _get_cff_version()
    cmake_version = _get_cmake_version()
    if tag_version != cff_version:
        sys.stderr.write("CFF file version does not match the current tag version")
        sys.exit(1)
    if tag_version != cmake_version:
        sys.stderr.write("Version specified in cmake does not match the current tag version")
        sys.exit(1)
