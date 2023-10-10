# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import sys
import string
import subprocess
from os.path import abspath, dirname, join, exists, relpath


THIS_DIR = dirname(__file__)
DOC_DIR = dirname(THIS_DIR)
TOP_LEVEL_DIR = dirname(DOC_DIR)
MAIN_README = abspath(join(TOP_LEVEL_DIR, "README.md"))
assert exists(MAIN_README)


def _get_commit_sha() -> str:
    return subprocess.run(
        "git rev-list --max-count=1 HEAD".split(" "), capture_output=True, text=True, check=True
    ).stdout.strip("\n")


def _filter_characters(text: str) -> str:
    return "".join(filter(lambda c: c not in string.punctuation and c not in string.digits, text))


def _add_header_label(line: str) -> str:
    if not line.startswith("#") or line.startswith("#include"):
        return line
    line = line.rstrip("\n")
    label = _filter_characters(line)
    label = label.strip(" ").replace(" ", "-").lower()
    return f"{line} {{#{label}}}\n"


def _handle_relative_urls(line: str, file_path: str) -> str:
    pieces = line.split("<!-- DOXYGEN_MAKE_ABSOLUTE -->")
    if len(pieces) == 1:
        return line

    def _make_absolute_url(p: str) -> str:
        rel_path = p.split("](", maxsplit=1)[1].split(")", maxsplit=1)[0]
        rel_path_from_top_level = relpath(join(dirname(file_path), rel_path.strip()), TOP_LEVEL_DIR)
        return p.replace(
            f"({rel_path})",
            f"(https://github.com/dglaeser/gridformat/tree/{_get_commit_sha()}/{rel_path_from_top_level})"
        )
    return pieces[0] + "".join(_make_absolute_url(p) for p in pieces[1:])


def _process_line(line: str, file_path: str) -> str:
    hint_key = "<!-- DOXYGEN_ONLY"
    if hint_key in line:
        return line.replace(hint_key, "").replace("-->", "").strip()
    else:
        return _add_header_label(
            _handle_relative_urls(line, file_path)
        )


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
content = subprocess.run(
    ["perl", "-0777", "-p", join(THIS_DIR, "markdown_math_filter.pl"), filepath],
    capture_output=True,
    text=True
).stdout

if filepath == MAIN_README:
    content = f"# Introduction\n\n{content}"

print("\n".join([
    _process_line(line, filepath)
    for line in _remove_leading_comments_and_empty_lines(content.split("\n"))
]))
