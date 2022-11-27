# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

"""Adds or modifies the license text in all version-controlled source files (of known type)"""

from os import walk
from os.path import isdir, splitext, join
from typing import List


def _cpy_right_text() -> str:
    return "SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>"


def _license_id_text() -> str:
    # split because reuse linter gets confused with finding this license statement
    return "SPDX" + "-License-Identifier: GPL-3.0-or-later"


def _years_string() -> str:
    return "2022"


def _is_cpp_file(filename: str) -> bool:
    return splitext(filename)[1] in [".hpp", ".cpp"]


def _is_py_file(filename: str) -> bool:
    return splitext(filename)[1] == ".py"


def _is_cmake_file(filename: str) -> bool:
    return filename == "CMakeLists.txt" or splitext(filename)[1] == ".cmake"


def _has_valid_license_statement(path: str) -> bool:
    with open(path) as src_file:
        has_cpy_right, has_license = False, False
        for line in src_file:
            if "FileCopyrightText" in line:
                has_cpy_right = True
            elif "License-Identifier" in line:
                has_license = True
        return has_cpy_right and has_license


def _add_license_statement(path: str, comment_prefix: str) -> None:
    if _has_valid_license_statement(path):
        return
    print(f"Adding license info to {path}")
    content = open(path).readlines()
    if content and content[0] != "":
        content.insert(0, "\n")
    content.insert(0, f"{comment_prefix} {_license_id_text()}\n")
    content.insert(0, f"{comment_prefix} {_cpy_right_text()}\n")
    with open(path, "w") as src_file:
        src_file.write("".join(content))


def _scanned_folders() -> List[str]:
    return ["gridformat", "test", "cmake"]


def _is_top_folder():
    return all(isdir(_f) for _f in _scanned_folders())


if __name__ == "__main__":
    if not _is_top_folder:
        raise IOError("This script must be called from the top-level of the project")

    for base in _scanned_folders():
        for root, folders, files in walk(base):
            for cpp_header in filter(_is_cpp_file, files):
                _add_license_statement(join(root, cpp_header), "//")
            for py_file in filter(_is_py_file, files):
                _add_license_statement(join(root, py_file), "#")
            for cmake_file in filter(_is_cmake_file, files):
                _add_license_statement(join(root, cmake_file), "#")
