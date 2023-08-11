# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from argparse import ArgumentParser
from subprocess import run
from shutil import copy
from os.path import exists, join, abspath, dirname
from os import mkdir, chdir, getcwd


CMAKE_LISTS = """
cmake_minimum_required(VERSION 3.22)
project(cli_test_data_generator)

include(FetchContent)
FetchContent_Declare(
    gridformat
    GIT_REPOSITORY https://github.com/dglaeser/gridformat
    GIT_TAG main
    GIT_PROGRESS true
    GIT_SHALLOW true
    GIR_SUBMODULES ""
    GIT_SUBMODULES_RECURSE OFF
)
FetchContent_MakeAvailable(gridformat)
add_executable(generate_data generate_data.cpp)
target_link_libraries(generate_data PRIVATE gridformat::gridformat)
"""

CPP_CODE = """
#include <gridformat/gridformat.hpp>

int main () {
    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {10, 12}};
    GridFormat::Writer writer{GridFormat::vti, grid};
    writer.set_meta_data("meta", "i am metadata");
    writer.set_point_field("pfield", [&] (const auto&) { return 1.0; });
    writer.set_cell_field("cfield", [&] (const auto&) { return 1.0; });
    writer.write("my_vti_file");
    return 0;
}
"""

def generate_test_file(cmake_args: list[str]) -> str:
    folder = "__readme_cli_test_folder"
    mkdir(folder)
    cwd = getcwd()
    chdir(folder)
    with open("CMakeLists.txt", "w") as cmake_file:
        cmake_file.write(CMAKE_LISTS)
    with open("generate_data.cpp", "w") as cpp_file:
        cpp_file.write(CPP_CODE)
    run(["cmake"] + cmake_args + ["-B", "build"], check=True)
    run(["cmake", "--build", "build"], check=True)
    run(["./build/generate_data"], check=True)
    chdir(cwd)
    return join(folder, "my_vti_file.vti")


def get_cli_snippets(readme_file: str) -> str:
    content = open(readme_file).read()
    return content.split("Command-line interface")[1].split("```cpp", maxsplit=1)[1].split("```", maxsplit=1)[0]


parser = ArgumentParser()
parser.add_argument("-c", "--cmake-args", required=False)
args = vars(parser.parse_args())

print(f"Testing cli code snippets")
readme_path = join(join(dirname(abspath(__file__)), ".."), "README.md")
if not exists(readme_path):
    raise IOError("Could not find readme")

cmake_args = args["cmake_args"]
test_filename = generate_test_file(cmake_args=cmake_args.split() if cmake_args is not None else [])
copy(test_filename, ".")
bash_script_name = "__cli_snippets_bash_script.sh"
with open(bash_script_name, "w") as bash_script:
    content = get_cli_snippets(readme_path)
    print("Using cli script:")
    print(content)
    bash_script.write(content)

run(["/bin/bash", bash_script_name], check=True)
