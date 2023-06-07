# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

import sys
import string
from os.path import abspath, dirname, join, exists


THIS_DIR = dirname(__file__)
DOC_DIR = dirname(THIS_DIR)
TOP_LEVEL_DIR = dirname(DOC_DIR)
MAIN_README = abspath(join(TOP_LEVEL_DIR, "README.md"))
assert exists(MAIN_README)


def _filter_characters(text: str) -> str:
    return "".join(filter(lambda c: c not in string.punctuation and c not in string.digits, text))


def _add_header_label(line: str) -> str:
    if not line.startswith("#"):
        return line
    line = line.rstrip("\n")
    label = _filter_characters(line)
    label = label.strip(" ").replace(" ", "-").lower()
    return f"{line} {{#{label}}}\n"


def _is_comment(line):
    return line.startswith("<!--")


def _remove_leading_comments_and_empty_lines(lines: list) -> list:
    start_index = 0
    for i in range(len(lines)):
        if lines[i] and not _is_comment(lines[i]):
            start_index = i
            break
    return lines[start_index:]


assert len(sys.argv[1]) > 1
filepath = abspath(sys.argv[1])
content = open(filepath, "r").read()

if filepath == MAIN_README:
    content = f"# Introduction\n\n{content}"

print("\n".join([
    _add_header_label(line)
    for line in _remove_leading_comments_and_empty_lines(content.split("\n"))
]))
