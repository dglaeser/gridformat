# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.a.glaeser@gmail.com>
# SPDX-License-Identifier: MIT

"""
Prints all files that have been modified between two commits.
"""

import subprocess
import argparse
import os


def prepare_repo() -> str:
    _folder_name = "_tmpgfmt"
    subprocess.run(["git", "clone", "https://github.com/dglaeser/gridformat", _folder_name], check=True, capture_output=True)
    subprocess.run(["git", "fetch", "origin"], check=True, capture_output=True, cwd=_folder_name)
    return _folder_name


def get_modified_files(source_tree: str, target_tree: str) -> list[str]:
    return subprocess.run(
        ["git", "diff", "--name-only", source_tree, target_tree],
        check=True, capture_output=True, text=True
    ).stdout.strip(" \n").split("\n")


def try_in(folder: str, action):
    cwd = os.getcwd()
    try:
        os.chdir(folder)
        result = action()
    except Exception as e:
        os.chdir(cwd)
        raise e
    os.chdir(cwd)
    return result


parser = argparse.ArgumentParser(description="Print the files that have been modified between two commits")
parser.add_argument("-s", "--source", required=True, help="The source git tree.")
parser.add_argument("-t", "--target", required=True, help="The target git tree.")
parser.add_argument(
    "-i", "--in-place",
    required=False,
    action="store_true",
    help="Use this flag if you want to detect the differences in-place. Per default, the script clones the repo."
)
args = vars(parser.parse_args())

source = args["source"]
target = args["target"]
modified_files = get_modified_files(source, target) if args["in_place"] else try_in(
    prepare_repo(), lambda s=source, t=target: get_modified_files(s, t)
)
print("\n".join(modified_files))
