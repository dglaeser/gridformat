# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

"""Adds or modifies the license text in all version-controlled source files (of known type)"""

from os import walk
from os.path import splitext, join, exists
from typing import Optional
from datetime import datetime
from argparse import ArgumentParser


def _cpy_right_text(years, authors: str) -> str:
    return f"SPDX-FileCopyrightText: {years} {authors}"


def _license_id_text(license: str) -> str:
    # split because reuse linter gets confused with finding this license statement
    return "SPDX" + f"-License-Identifier: {license}"


def _get_authors(filename: str) -> str:
    with open(filename) as sourcefile:
        for line in sourcefile:
            if "FileCopyrightText: " in line:
                return _remove_suffixes(line.split("FileCopyrightText: ", maxsplit=1)[1].strip())
    raise RuntimeError("Could not extract author information")


def _get_years(filename: str) -> str:
    with open(filename) as sourcefile:
        for line in sourcefile:
            if "FileCopyrightText" in line:
                return line.split("FileCopyrightText: ")[1].split(" ")[0]
    raise RuntimeError("Could not extract years information")


def _get_license(filename: str) -> str:
    with open(filename) as sourcefile:
        for line in sourcefile:
            if "License-Identifier: " in line:
                return _remove_suffixes(line.split("License-Identifier: ", maxsplit=1)[1].strip())
    raise RuntimeError("Could not extract license information")


def _remove_suffixes(text: str) -> str:
    return text.removesuffix("-->").strip()


def _update_license_info(filename: str, license_update: Optional[tuple[str, str]] = None) -> None:
    authors = _get_authors(filename)
    year = _get_years(filename)
    license = _get_license(filename)
    new_license = license if license_update is None else (
        license_update[1] if license_update[0] == license else license
    )

    new_years = year
    if "-" not in year and year != str(datetime.now().year):
        new_years = f"{year}-{datetime.now().year}"
    elif "-" in year and year.split("-")[1] != str(datetime.now().year):
        new_years = f"{year.split('-')[0]}-{datetime.now().year}"

    text = open(filename).read()
    text = text.replace(_cpy_right_text(year, authors), _cpy_right_text(new_years, authors))
    text = text.replace(_license_id_text(license), _license_id_text(new_license))
    with open(filename, "w") as new_file:
        new_file.write(text)


def _get_separate_license_file(filename: str) -> Optional[str]:
    if exists(f"{filename}.license"):
        return f"{filename}.license"


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-c", "--cur-license", required=False, help="Select a license that should be replaced")
    parser.add_argument("-n", "--new-license", required=False, help="Select a license with which to replace")
    parser.add_argument("-e", "--extension", required=False, help="Edit only the files with the given extension")
    args = vars(parser.parse_args())

    if args["cur_license"] is not None:
        assert args["new_license"] is not None

    license_update: Optional[tuple[str, str]] = None
    if args["cur_license"] and args["new_license"]:
        license_update = (args["cur_license"], args["new_license"])

    for root, folders, files in walk("."):
        for f in files:
            if args["extension"] and splitext(f)[1] != args["extension"]:
                continue

            f = join(root, f)
            if "LICENSES" in f or "doxygen" in f or ".git/" in f:
                continue

            license_file = _get_separate_license_file(f)
            if license_file is not None:
                f = license_file
            print(f"Processing file '{f}'")
            _update_license_info(f, license_update)
