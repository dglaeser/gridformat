# SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import sys
import subprocess
import argparse
import datetime

from os.path import join, abspath, dirname


CMAKE_LISTS_PATH = join(dirname(abspath(__file__)), "../CMakeLists.txt")
CFF_FILE_PATH = join(dirname(abspath(__file__)), "../CITATION.cff")


def _is_release_version_tag(tag: str) -> bool:
    if not tag.startswith("v"):
        return False
    tag = tag[1:]
    try:
        major, minor, _ = tag.split(".")
        _, _ = int(major), int(minor)
        return True
    except:
        return False


def _to_tag(version: str) -> str:
    tag = version if version.startswith("v") else f"v{version}"
    assert _is_release_version_tag(tag)
    return tag


def _run_and_capture(cmd: str | list[str]) -> str:
    if not isinstance(cmd, list):
        cmd = cmd.split()
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def _split_after(match: str, text: str) -> tuple[str, str]:
    before, after = text.split(match, maxsplit=1)
    return before + match, after


def _split_cmake_version() -> tuple[str, str, str]:
    with open(CMAKE_LISTS_PATH) as cmake_file:
        content = cmake_file.read()
        before1, tmp = _split_after("\nproject(", content)
        before2, version_and_rest = _split_after("VERSION ", tmp)
        version, rest = version_and_rest.split(")", maxsplit=1)
        return f"{before1}{before2}", version, f"){rest}"


def _get_cmake_version() -> str:
    return _split_cmake_version()[1]


def _set_cmake_version(new_version: str) -> None:
    assert _is_release_version_tag(f"v{new_version}")
    before, _, after = _split_cmake_version()
    with open(CMAKE_LISTS_PATH, "w") as cmake_file:
        cmake_file.write(f"{before}{new_version}{after}")


def _split_cff_at(key: str) -> tuple[str, str, str]:
    with open(CFF_FILE_PATH) as cff_file:
        content = cff_file.read()
        before, value_and_rest = _split_after(key, content)
        value, rest = value_and_rest.split("\n", maxsplit=1)
        return before, value, f"\n{rest}"


def _get_cff_version() -> str:
    return _split_cff_at("\nversion: ")[1]


def _set_cff_version(new_version: str) -> None:
    assert _is_release_version_tag(f"v{new_version}")
    before, _, after = _split_cff_at("\nversion: ")
    with open(CFF_FILE_PATH, "w") as cff_file:
        cff_file.write(f"{before}{new_version}{after}")


def _get_cff_release_date() -> datetime.date:
    return datetime.date.fromisoformat(_split_cff_at("\ndate-released: ")[1].strip("'"))


def _set_cff_release_date(date: datetime.date) -> None:
    before, _, after = _split_cff_at("\ndate-released: ")
    with open(CFF_FILE_PATH, "w") as cff_file:
        cff_file.write(f"{before}'{date}'{after}")


def _get_tag_date(tag: str) -> datetime.date:
    tag_date = _run_and_capture(f"git log {tag}").split("\nDate: ", maxsplit=1)[1].split("\n", maxsplit=1)[0]
    tag_date = " ".join(tag_date.split()[:-1])  # remove timezone
    return datetime.datetime.strptime(tag_date, "%a %b %d %H:%M:%S %Y").date()


def _is_branch(branch: str) -> bool:
    try:
        _run_and_capture(f"git show-ref --verify refs/heads/{branch}")
        return True
    except:
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--version", required=True,
        help="The version to be set or checked"
    )
    parser.add_argument(
        "-c", "--check-only", action="store_true",
        help="Use this flag to check version consistency only"
    )
    parser.add_argument(
        "-s", "--skip-tag", action="store_true",
        help="Use this if the git tag should not be created or checked"
    )
    args = vars(parser.parse_args())

    tag = _to_tag(args["version"])
    version = tag[1:]
    if args["check_only"]:
        if not args["skip_tag"]:
            branch = _run_and_capture("git rev-parse --abbrev-ref HEAD")
            _run_and_capture(f"git checkout {tag}")
            cff_date = _get_cff_release_date()
            tag_date = _get_tag_date(tag)
            if _is_branch(branch):
                _run_and_capture(f"git switch {branch}")
            if cff_date != tag_date:
                sys.stderr.write("Tag date does not match cff release date")
                sys.exit(1)
        cff_version = _get_cff_version()
        cmake_version = _get_cmake_version()
        if cff_version != version:
            sys.stderr.write(f"Version specified in CFF file does not match the given version ({cff_version})\n")
            sys.exit(1)
        if cmake_version != version:
            sys.stderr.write(f"Version specified in cmake does not match the current tag version ({cmake_version})\n")
            sys.exit(1)
    else:
        _set_cmake_version(version)
        _set_cff_version(version)
        _set_cff_release_date(datetime.datetime.now().date())
        if not args["skip_tag"]:
            subprocess.run([
                "git", "commit", "-m", f"bump version to {tag}", CFF_FILE_PATH, CMAKE_LISTS_PATH
            ], check=True)
            _run_and_capture(["git", "tag", f"{tag}", "HEAD", "-a", "-m", f"Tag version {version}"])
