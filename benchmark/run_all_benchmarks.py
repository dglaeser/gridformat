# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import subprocess
import argparse
import shutil
import os

parser = argparse.ArgumentParser()
parser.add_argument("-o", "--out-folder", required=True, help="folder where to place the result files")
args = vars(parser.parse_args())

subprocess.run(["make"], check=True)
subprocess.run(["ctest", "-V"], check=True)

files = [
    os.path.join(root, file)
    for root, _, files in os.walk(".")
    for file in filter(
        lambda f: os.path.exists(os.path.join(root, "CMakeFiles")) and os.path.splitext(f)[1] == ".csv",
        files
    )
]

out_folder = args["out_folder"]
shutil.rmtree(out_folder, ignore_errors=True)
os.makedirs(out_folder, exist_ok=True)
for file in files:
    shutil.copyfile(file, os.path.join(out_folder, os.path.basename(file)))
