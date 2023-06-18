# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from argparse import ArgumentParser
from subprocess import run
from os.path import exists, join, abspath, dirname
from os import mkdir, chdir


ORIGIN = "https://github.com/dglaeser/gridformat"


def _get_cmake_lists_content(readme_file, branch: str) -> str:
    content = open(readme_file).read()
    content = content.split("Quick Start")[1].split("```cmake", maxsplit=1)[1].split("```", maxsplit=1)[0]
    return content.replace("GIT_TAG main", f"GIT_TAG {branch}")


def _get_app_content(readme_file) -> str:
    content = open(readme_file).read()
    return content.split("Quick Start")[1].split("```cpp", maxsplit=1)[1].split("```", maxsplit=1)[0]


parser = ArgumentParser()
parser.add_argument("-e", "--extra-cmake-args", required=False)
parser.add_argument("-r", "--ref", required=False, default="main")
parser.add_argument("-o", "--origin", required=False, default="https://github.com/dglaeser/gridformat")
args = vars(parser.parse_args())

print(f"Testing quick start instructions from ref '{args['ref']}'")
readme_path = join(join(dirname(abspath(__file__)), ".."), "README.md")
if not exists(readme_path):
    raise IOError("Could not find readme")

mkdir("readme_quick_start_test")
chdir("readme_quick_start_test")

with open("CMakeLists.txt", "w") as cmake_file:
    cmake_file.write(_get_cmake_lists_content(readme_path, args["ref"]).replace(ORIGIN, args["origin"]))
with open("my_app.cpp", "w") as cpp_file:
    cpp_file.write(_get_app_content(readme_path))

extra_cmake_args = args["extra_cmake_args"]
run(
    ["cmake"]
    + (extra_cmake_args.split() if extra_cmake_args else [])
    + ["-B", "build"]
    , check=True
)

chdir("build")
run(["make", "my_app"], check=True)
run(["./my_app"], check=True)
