# SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import sys
import argparse

from os.path import join, abspath, dirname, exists

CHANGELOG_VERSION_HEADER_PREFIX = "# `GridFormat` "


def _get_changelog_content_for(version: str) -> str:
    with open(join(dirname(abspath(__file__)), "../CHANGELOG.md")) as changelog:
        _, content = changelog.read().split(f"{CHANGELOG_VERSION_HEADER_PREFIX}{version}", maxsplit=1)
        # strip rest of the header line
        _, content = content.split("\n", maxsplit=1)
        return content.split(f"{CHANGELOG_VERSION_HEADER_PREFIX}")[0].strip("\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--version", required=True, help="specify the version for which to create the description")
    parser.add_argument("-o", "--out-file", required=True, help="specify the name of the file in which to write the description")
    args = vars(parser.parse_args())

    if exists(args["out_file"]):
        sys.stderr.write("Description file already exists!")
        sys.exit(1)

    version = args["version"]
    version = version[1:] if version.startswith("v") else version
    with open(args["out_file"], "w") as description:
        description.write(_get_changelog_content_for(version))
