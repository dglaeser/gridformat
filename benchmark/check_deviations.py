# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import argparse
import numpy
import sys
import os


try:
    import colorama
    colorama.init()

    def _as_error(msg: str) -> str:
        return colorama.Style.BRIGHT + colorama.Fore.RED + str(msg) + colorama.Style.RESET_ALL
    def _as_success(msg: str) -> str:
        return colorama.Style.BRIGHT + colorama.Fore.GREEN + str(msg) + colorama.Style.RESET_ALL
except ImportError:
    def _as_error(msg: str) -> str:
        return str(msg)
    def _as_success(msg: str) -> str:
        return str(msg)


def average(results: list[object]) -> float:
    results = list(map(lambda r: float(r), filter(lambda r: r != b"-", results)))
    return sum(results)/len(results)


def compare(result_file: str, reference_file: str, tolerance: float) -> int:
    results = numpy.genfromtxt(result_file, delimiter=",", dtype=object, names=True)
    reference = numpy.genfromtxt(reference_file, delimiter=",", dtype=object, names=True)
    all_names = set(results.dtype.names[1:]).union(set(reference.dtype.names[1:]))
    for benchmark in all_names:
        avg_result = average(results[benchmark])
        avg_reference = average(reference[benchmark])
        rel_diff = (avg_result - avg_reference)/max(avg_reference, avg_result)
        passed = rel_diff <= tolerance
        print("Relative deviation of average for '{}': {}".format(
            benchmark,
            _as_success(rel_diff) if passed else _as_error(rel_diff)
        ))
    return 0


parser = argparse.ArgumentParser()
parser.add_argument("-f", "--folder", required=True, help="folder with benchmark result files")
parser.add_argument("-r", "--reference-folder", required=True, help="folder with benchmark reference result files")
parser.add_argument("-t", "--relative-tolerance", required=False, default="0.02", help="tolerance for 'deteriorated' performance")
args = vars(parser.parse_args())

folder = args["folder"]
ref_folder = args["reference_folder"]
files = set([f for _, _, files in os.walk(folder) for f in files]).union(
    set([f for _, _, files in os.walk(ref_folder) for f in files])
)

ret_code = 0
for f in files:
    ret_code += compare(
        os.path.join(folder, f),
        os.path.join(ref_folder, f),
        float(args["relative_tolerance"])
    )

print(f"Exit code: {ret_code}")
sys.exit(ret_code)
