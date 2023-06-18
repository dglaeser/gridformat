# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from os.path import join, dirname, abspath
from argparse import ArgumentParser


def _get_pkg_versions_from_readme(readme_path: str) -> dict:
    content = open(readme_path).read()

    def _split_version(pkg_name: str) -> str:
        return content.split(f"tested {pkg_name} version:")[1].split(")")[0].strip()

    result = {}
    result["dune"] = _split_version("dune")
    result["dealii"] = _split_version("deal.ii")
    result["cgal"] = _split_version("cgal")
    result["dolfinx"] = _split_version("dolfinx")
    result["mfem"] = _split_version("mfem")
    return result;


def _get_pkg_versions_from_cmake_log(cmake_log: str) -> dict:
    content = open(cmake_log).read()

    def _split_version(pkg_name: str, last_word: str = "version") -> str:
        return content.split(f"Testing {pkg_name} traits")[1].split(last_word)[1].split("\n")[0].strip()

    def _split_dune_version() -> str:
        version_line = _split_version("Dune", "versions")
        version = version_line.split()[0]
        # ensure the versions for all dune packages are the same
        assert sum(1 for word in version_line.split() if word == version) == 3
        return version

    result = {}
    result["dune"] = _split_dune_version()
    result["dealii"] = _split_version("deal.ii")
    result["cgal"] = _split_version("CGAL")
    result["dolfinx"] = _split_version("dolfinx")
    result["mfem"] = _split_version("mfem")
    return result;


parser = ArgumentParser()
parser.add_argument("-l", "--cmake-log-file", required=True)
args = vars(parser.parse_args())

readme_versions = _get_pkg_versions_from_readme(join(join(dirname(abspath(__file__)), ".."), "README.md"))
cmake_log_versions = _get_pkg_versions_from_cmake_log(args["cmake_log_file"])

print("Versions found by cmake: ", cmake_log_versions)
print("Versions stated in the readme: ", readme_versions)
