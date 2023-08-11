# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from argparse import ArgumentParser
from os import listdir, getenv
from os.path import abspath, dirname, join, exists, isfile
from subprocess import run, CalledProcessError
from fnmatch import fnmatch
from sys import exit


def run_and_catch_error_code(cmd: list) -> tuple[Exception | None, int]:
    try:
        run(cmd, check=True)
    except CalledProcessError as e:
        return e, e.returncode
    return None, 0


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-c", "--command", required=True)
    parser.add_argument("-r", "--regex", required=True)
    parser.add_argument("-sm", "--skip-metadata", required=False, action="store_true")
    args = vars(parser.parse_args())

    # fnmatch does not support unix patterns like prefix*+(suffix1|suffix2) as supported by bash...
    # Therefore, we accept regexes in the form of regex1||regex2, split at the ||, and match against all regexes...
    # Alternatively, we could make regex an argument that can occur multiple times, but this would require quite some
    # changes in the cmake scripts...
    regexes = args["regex"].split("||")
    def _matches(filename: str) -> bool:
        return any(fnmatch(filename, _r) for _r in regexes)

    mypath = abspath(__file__)
    myfolder = dirname(mypath)
    vtk_check_script = join(myfolder, "test_vtk_file.py")
    if not exists(vtk_check_script):
        raise RuntimeError("Could not find vtk test script")

    e, ret_code = run_and_catch_error_code(args["command"].split(" "))
    if ret_code not in [0, 42]:
        print(f"Error upon test execution: {e}")
        exit(ret_code)

    ret_codes = [ret_code]
    env_variable_control_name = "GRIDFORMAT_REGRESSION_SAMPLES_ONLY"
    take_single_sample = True if getenv(env_variable_control_name, "false").lower() in ["true", "1"] else False
    for _file in filter(lambda _f: isfile(_f), listdir(".")):
        if _matches(_file):
            print(f"Regression testing '{_file}'")
            ret_codes.append(run_and_catch_error_code(
                ["python3", vtk_check_script, "-p", _file] + (
                    ["--skip-metadata"] if args["skip_metadata"] else []
                )
            )[1])

            if take_single_sample:
                print(f"Stopping after first file because '{env_variable_control_name}' was set")
                break

    for c in ret_codes:
        if c != 0 and c != 42:
            exit(c)
    exit(max(ret_codes))
