# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from argparse import ArgumentParser
from os import listdir, getenv
from os.path import abspath, dirname, join, exists, isfile
from subprocess import run, CalledProcessError
from fnmatch import fnmatch
from sys import exit


def run_and_catch_error_code(cmd: list) -> int:
    try:
        run(cmd, check=True)
    except CalledProcessError as e:
        return e.returncode
    return 0


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-c", "--command", required=True)
    parser.add_argument("-r", "--regex", required=True)
    parser.add_argument("-sm", "--skip-metadata", required=False, action="store_true")
    args = vars(parser.parse_args())

    mypath = abspath(__file__)
    myfolder = dirname(mypath)
    vtk_check_script = join(myfolder, "test_vtk_file.py")
    if not exists(vtk_check_script):
        raise RuntimeError("Could not find vtk test script")

    ret_code = run_and_catch_error_code(args["command"].split(" "))
    env_variable_control_name = "GRIDFORMAT_REGRESSION_SAMPLES_ONLY"
    take_single_sample = True if getenv(env_variable_control_name, "false").lower() in ["true", "1"] else False
    for _file in filter(lambda _f: isfile(_f), listdir(".")):
        if fnmatch(_file, f"{args['regex']}"):
            print(f"Regression testing '{_file}'")
            ret_code = max(ret_code, run_and_catch_error_code(
                ["python3", vtk_check_script, "-p", _file] + (
                    ["--skip-metadata"] if args["skip_metadata"] else []
                )
            ))

            if take_single_sample:
                print(f"Stopping after first file because '{env_variable_control_name}' was set")
                break
    exit(ret_code)
