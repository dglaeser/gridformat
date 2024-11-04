# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.a.glaeser@gmail.com>
# SPDX-License-Identifier: MIT

"""
Prints the commands that can be used to compile or test the tests listed in a file,
e.g. previously determined by the "detect_affected_tests.py" script.
"""

import sys
import argparse


def make_regex(tests: list[str]) -> str:
    return "|".join(f"^{t}$" for t in tests)


def print_command(cmd: str) -> None:
    print(cmd, end="")


parser = argparse.ArgumentParser(
    description="Prints the command for building/testing the list of tests specified in the provided file. "
                "If the file solely contains 'all', a command for running all tests is printed."
)
parser.add_argument(
    "-b", "--build",
    required=False,
    action='store_true',
    help="Use this flag to print the build command"
)
parser.add_argument(
    "-t", "--test",
    required=False,
    action='store_true',
    help="Use this flag to print the test command"
)
parser.add_argument(
    "-a", "--ctest-args",
    required=False,
    default="",
    help="String containing other arguments to be forwarded to ctest"
)
parser.add_argument(
    "-f", "--tests-file",
    required=True,
    help="Name of the file with the list of tests to be run"
)
parser.add_argument(
    "-m", "--memcheck",
    required=False,
    action="store_true",
    help="Use this flag if you want to build the memcheck target (yields no test command)"
)
parser.add_argument(
    "-l", "--all-targets-list",
    required=False,
    default="",
    help="Path to a file containing all available targets (e.g. created with ' cmake --build build --target help')"
)
args = vars(parser.parse_args())

do_build = args["build"]
do_test = args["test"]
if not do_build and not do_test:
    sys.stderr.write("Either the 'build' or 'test' flag must be specified")
    sys.exit(1)
if do_build and do_test:
    sys.stderr.write("One of the 'build' or 'test' flags must be specified")
    sys.exit(1)

tests = [n for n in open(args["tests_file"]).read().strip(" \n").split("\n") if n]
ctest_args = args["ctest_args"]
is_memcheck = args["memcheck"]

memcheck_test_cmd = f'echo "Nothing to do; Memcheck tests run with the build command."'
no_tests_msg = f"echo 'Nothing to {'test' if do_test else 'build'}; empty test selection provided...'"

if tests == ["all"]:
    if is_memcheck:
        print_command(memcheck_test_cmd if do_test else f"make memcheck")
    else:
        print_command(f"ctest {ctest_args}" if do_test else f"make build_tests")
elif tests == []:
    print_command(no_tests_msg)
else:
    if is_memcheck:
        if do_test:
            print_command(memcheck_test_cmd)
        if not args["all_targets_list"]:
            sys.stderr.write("For memcheck you need to pass a list of all available targets (see help)")
            sys.exit(1)
        all_targets = open(args["all_targets_list"]).read()
        memcheck_tests = [f'{t}_memcheck' for t in tests]
        memcheck_tests = list(filter(lambda t: t in all_targets, memcheck_tests))
        print_command(f"make {' '.join(t for t in memcheck_tests)}" if memcheck_tests else no_tests_msg)
    else:
        print_command(f"ctest {ctest_args} -R {make_regex(tests)}" if do_test else f"make {' '.join(tests)}")
