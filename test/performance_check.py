# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import subprocess
import datetime
import argparse
import shutil
import sys
import os


def run_benchmarks(opts: dict, folder: str, output_folder: str) -> None:
    shutil.rmtree(os.path.join(folder, "build"), ignore_errors=True)
    subprocess.run([
        "cmake", f"-DCMAKE_C_COMPILER={opts['c_compiler']}",
                 f"-DCMAKE_CXX_COMPILER={opts['cxx_compiler']}",
                 "-DCMAKE_BUILD_TYPE=Release",
                 f"-DCMAKE_PREFIX_PATH='{opts['prefix_path']}'",
                 f"-DGRIDFORMAT_FETCH_TREE={opts['tree']}",
                 f"-DGRIDFORMAT_ORIGIN={opts['origin']}",
                 "-B", "build"
        ],
        check=True,
        cwd=folder
    )
    subprocess.run(
        [sys.executable, "../run_all_benchmarks.py", "-o", output_folder],
        check=True,
        cwd=os.path.join(folder, "build")
    )


if __name__ == "__main__":
    default_url = "https://github.com/dglaeser/gridformat"
    my_path = os.path.abspath(__file__)
    base_path = os.path.join(os.path.dirname(my_path), "..")
    if not os.path.samefile(os.path.abspath(os.getcwd()), base_path):
        sys.stderr.write("This script should be run from the root of the repository")
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Run the benchmarks with two different gridformat versions and compare the measured run times."
    )
    parser.add_argument("-c", "--c-compiler", required=True, help="C compiler to be used")
    parser.add_argument("-cxx", "--cxx-compiler", required=True, help="C++ compiler to be used")
    parser.add_argument("-p", "--prefix-path", required=False, default="", help="Path where cmake can find dependencies")
    parser.add_argument("-t", "--tree", required=True, help="The git tree to use for running the benchmark")
    parser.add_argument("-o", "--origin", required=False, default=default_url, help="The url from where to clone the tree")
    parser.add_argument("-rt", "--reference-tree", required=False, default="main", help="The git tree to use for the reference run")
    parser.add_argument("-ro", "--reference-origin", required=False, default=default_url, help="The url from where to clone the reference tree")
    parser.add_argument("-tol", "--relative-tolerance", required=False, default=0.02, help="Tolerance for 'deteriorated' performance")
    parser.add_argument("-f", "--out-folder", required=False, help="Folder where to place the results")
    parser.add_argument("-s", "--summary-file", required=False, default="", help="File into which to put a summary of the results")
    parser.add_argument("--print-only", required=False, action="store_true", help="if set, the exit code is independent of the results")
    args = vars(parser.parse_args())

    out_folder = args["out_folder"] or "performance_check_run_" + datetime.datetime.now().strftime("%Y-%m-%d-%M:%S")
    res_folder = os.path.join(os.path.abspath(out_folder), "results")
    ref_folder = os.path.join(os.path.abspath(out_folder), "reference")
    run_benchmarks(
        {
            "c_compiler": args["c_compiler"],
            "cxx_compiler": args["cxx_compiler"],
            "prefix_path": args["prefix_path"],
            "tree": args["tree"],
            "origin": args["origin"]
        },
        "benchmark",
        res_folder
    )
    run_benchmarks(
        {
            "c_compiler": args["c_compiler"],
            "cxx_compiler": args["cxx_compiler"],
            "prefix_path": args["prefix_path"],
            "tree": args["reference_tree"],
            "origin": args["reference_origin"]
        },
        "benchmark",
        ref_folder
    )
    subprocess.run([
        sys.executable, "benchmark/check_deviations.py",
                        "-f", res_folder,
                        "-r", ref_folder,
                        "-t", str(args["relative_tolerance"]),
                        "-s", args["summary_file"]
        ],
        check=bool(args["print_only"])
    )
