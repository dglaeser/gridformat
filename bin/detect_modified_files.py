# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.a.glaeser@gmail.com>
# SPDX-License-Identifier: MIT

"""
Prints all files that have been modified between two commits.
"""

import subprocess
import argparse
import shutil


def prepare_repo_at(
    directory: str,
    source_remote: str,
    target_remote: str
) -> None:
    subprocess.run(["git", "clone", source_remote, directory], check=True)
    subprocess.run(["git", "fetch", "origin"], check=True, cwd=directory)
    subprocess.run(["git", "remote", "add", "target", target_remote], check=True, cwd=directory)
    subprocess.run(["git", "fetch", "target"], check=True, cwd=directory)


def is_branch(name: str, remote: str, repo_dir: str) -> bool:
    try:
        subprocess.run(
            ["git", "show-ref", "--verify", f"refs/remotes/{remote}/{name}"],
            check=True,
            cwd=repo_dir
        )
        print(f"Found '{name}' to be a branch")
        return True
    except:
        print(f"'{name}' is not a branch")
        return False


def get_modified_files_in(repo_dir: str, source_tree: str, target_tree: str) -> list[str]:
    return subprocess.run(
        ["git", "diff", "--name-only", source_tree, target_tree],
        check=True,
        capture_output=True,
        text=True,
        cwd=repo_dir
    ).stdout.strip(" \n").split("\n")


parser = argparse.ArgumentParser(description="Print the files that have been modified between two commits")
parser.add_argument("-s", "--source", required=True, help="The source git tree.")
parser.add_argument("-t", "--target", required=True, help="The target git tree.")
parser.add_argument("-sr", "--source-remote", required=True, help="The url of the remote hosting the source tree")
parser.add_argument("-tr", "--target-remote", required=True, help="The url of the remote hosting the target tree")
parser.add_argument("-o", "--out-file", required=True, help="The name of the file in which to write the list of modified files.")

args = vars(parser.parse_args())

repo_dir = "_tmpgfmt"
print(f"Cloning gridformat into {repo_dir} (if folder exists it will be deleted)")
shutil.rmtree(repo_dir, ignore_errors=True)
prepare_repo_at(
    repo_dir,
    source_remote=args["source_remote"],
    target_remote=args["target_remote"]
)

print("Detecting modified files")
is_source_branch = is_branch(args['source'], "origin", repo_dir)
is_target_branch = is_branch(args['target'], "target", repo_dir)
modified_files = get_modified_files_in(
    repo_dir,
    source_tree=f"origin/{args['source']}" if is_source_branch else args['source'],
    target_tree=f"target/{args['target']}" if is_target_branch else args['target']
)

print("Detected the following modified files:")
print(" -", "\n - ".join(modified_files))
with open(args["out_file"], "w") as outfile:
    outfile.write("\n".join(modified_files))

print(f"Cleaning up cloned repo in {repo_dir}")
shutil.rmtree(repo_dir)
