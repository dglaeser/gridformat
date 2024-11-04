# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.a.glaeser@gmail.com>
# SPDX-License-Identifier: MIT

"""
Can be called from the build directory to find out which tests are affected by changes to files
Note: this assumes that the targets and test names are identical. In case we change this one day,
      this script should fail such that we detect the issue in the CI.
"""

import subprocess
import argparse
import json

from os import getcwd
from os.path import abspath, join, exists

try:
    import colorama
    colorama.init()

    def in_green(text: str) -> str:
        return colorama.Fore.GREEN + text + colorama.Style.RESET_ALL

    def in_red(text: str) -> str:
        return colorama.Fore.RED + text + colorama.Style.RESET_ALL

except ImportError:
    def in_green(text: str) -> str:
        return text

    def in_red(text: str) -> str:
        return text


_TOP_DIR = abspath(join(getcwd(), ".."))


def is_build_dir() -> bool:
    return exists(join(getcwd(), "CMakeCache.txt"))


def find_all_test_names() -> list[str]:
    output = json.loads(
        subprocess.run(["ctest", "--show-only=json-v1"], check=True, capture_output=True, text=True).stdout
    )
    return [data["name"] for data in output["tests"]]


def get_build_command(test_name: str) -> str:
    output = subprocess.run(["make", "--dry-run", test_name], check=True, capture_output=True, text=True).stdout
    for line in output.split("\n"):
        if line.startswith("cd ") and line.endswith(".cpp"):
            return line
    raise RuntimeError(f"Could not find build command for {test_name}")


def get_included_headers(test_name: str, verbosity: int = 0, line_prefix: str = "", test_prefix: str = "") -> list[str]:
    if verbosity > 1:
        print(test_prefix)
        print(f"{line_prefix}Finding headers for test {test_name}")
    build_command = get_build_command(test_name)
    folder_cmd, compile_command = build_command.split("&&", maxsplit=1)
    build_dir = folder_cmd.split("cd ", maxsplit=1)[1].strip()
    compiler, compiler_args = compile_command.strip().split(" ", maxsplit=1)
    assert compiler.rsplit("-", maxsplit=1)[0].endswith("++")
    composed_cmd = f"{compiler} {compiler_args} -MM -H".split(" ")
    output = subprocess.run(composed_cmd, check=True, capture_output=True, text=True, cwd=build_dir).stderr
    headers = [line.strip(" .") for line in output.split("\n") if line.startswith(".")]
    headers = list(filter(lambda h: _TOP_DIR in h, headers))
    if verbosity > 1:
        print("\n".join(f"{line_prefix} - {h}" for h in headers))
    return headers


def is_affected(test_name: str, modified_files: set, verbosity: int = 0) -> bool:
    if verbosity > 0:
        print(f"Checking if {test_name} is affected ... ", end="")
    headers = set(get_included_headers(test_name, verbosity, test_prefix="\n", line_prefix="\t"))
    affected = len(modified_files.intersection(headers)) > 0
    if verbosity > 0:
        print(in_red("YES") if affected else in_green("NO"))
    return affected


def read_modified_files(input_file: str, verbosity: int = 0) -> set[str]:
    modified_files = set(join(_TOP_DIR, p) for p in open(input_file).read().split("\n"))
    if verbosity > 0:
        print("Modified files:")
        print("\n".join(f" - {f}" for f in modified_files))
    return modified_files


def has_cmake_file(modified_files: set[str]) -> bool:
    def _is_cmake_file(path: str) -> bool:
        return path.endswith("CMakeLists.txt")
    return any(_is_cmake_file(p) for p in modified_files)


def find_affected_tests(modified_files: set[str], verbosity: int = 0) -> list[str]:
    affected = []
    discarded = []
    for test_name in find_all_test_names():
        if is_affected(test_name, modified_files, verbosity=verbosity):
            affected.append(test_name)
        else:
            discarded.append(test_name)

    if verbosity > 0:
        if affected:
            print(f"Affected tests ({len(affected)}):")
            print("\n".join(f" - {t}" for t in affected))
        else:
            print("No affected tests")
        if discarded:
            print(f"Discarded tests ({len(discarded)}):")
            print("\n".join(f" - {t}" for t in discarded))
        else:
            print("No discarded tests")

    return affected


def write_output_file(affected_tests: list[str], out_file: str) -> None:
    with open(out_file, "w") as output_file:
        output_file.write("\n".join(affected_tests))


if not is_build_dir():
    raise RuntimeError("This script must be called from the top level of the build directory")

parser = argparse.ArgumentParser(description="Find tests affected by modified headers")
parser.add_argument("-i", "--input-file", required=True, help="file with a list of modified files separated by lines")
parser.add_argument("-o", "--out-file", required=True, help="file to which to write the names of affected tests")
parser.add_argument("-v", "--verbosity", required=False, default=1)
args = vars(parser.parse_args())

verbosity = int(args["verbosity"])
modified_files = read_modified_files(args["input_file"])
if has_cmake_file(modified_files):
    if verbosity > 0:
        print("Writing 'all' target because of modified cmake file(s)")
    write_output_file(["all"], args["out_file"])
else:
    subprocess.run(["make", "clean"], check=True)
    affected_tests = find_affected_tests(modified_files, verbosity)
    write_output_file(affected_tests, args["out_file"])
    subprocess.run(["make", "clean"], check=True)
