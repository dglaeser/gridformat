# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import subprocess
import argparse
import shutil
import sys
import os


PACKAGES = {
    "cgal": "5.2.2",
    "dolfinx": "0.6.0",
    "dune": "2.9",
    "mfem": "4.5.2",
    "doxygen": "Release_1_9_6"
}


def _clone_sources(origin: str, branch: str, foldername: str | None = None) -> None:
    subprocess.run(
        ["git", "clone", "--depth=1", f"--branch={branch}", origin]
        + ([foldername] if foldername is not None else [])
        , check=True
    )


def _install_from_source(opts: dict, subdir: str | None = None) -> None:
    def _has_opt(name: str) -> bool:
        return opts.get(name) is not None

    tmp_folder = "gfmt_install_tmp"
    _clone_sources(opts["origin"], opts["branch"], foldername=tmp_folder)
    cwd = os.getcwd()
    os.chdir(tmp_folder)
    if subdir:
        os.chdir(subdir)

    c_compiler_arg = [f"-DCMAKE_C_COMPILER={opts['c_compiler']}"] if _has_opt("c_compiler") else []
    cxx_compiler_arg = [f"-DCMAKE_CXX_COMPILER={opts['cxx_compiler']}"] if _has_opt("cxx_compiler") else []
    prefix_arg = [f"-DCMAKE_INSTALL_PREFIX={opts['install_prefix']}"] if _has_opt("install_prefix") else []
    subprocess.run(["cmake"] + prefix_arg + c_compiler_arg + cxx_compiler_arg + ["-B", "build"], check=True)
    subprocess.run(["cmake", "--build", "build", "-j4"], check=True)
    subprocess.run(["cmake", "--install", "build"], check=True)

    os.chdir(cwd)
    shutil.rmtree(tmp_folder)


def _install_pkg(name, opts: dict) -> None:
    print(f"Installing {name}")
    if name == "cgal":
        _install_from_source({
            "origin": "https://github.com/CGAL/cgal.git",
            "branch": f"v{PACKAGES['cgal']}"
        } | opts)
    elif name == "dolfinx":
        _install_from_source({
            "origin": "https://github.com/FEniCS/basix.git",
            "branch": f"v{PACKAGES['dolfinx']}"
        } | opts)
        _install_from_source({
            "origin": "https://github.com/FEniCS/dolfinx.git",
            "branch": f"v{PACKAGES['dolfinx']}"
        } | opts, subdir="cpp")
    elif name == "dune":
        cwd = os.getcwd()
        os.mkdir("dune_libs")
        os.chdir("dune_libs")
        with open("dune.opts", "w") as opts_file:
            prefix_arg = str(
                f"-DCMAKE_INSTALL_PREFIX={opts['install_prefix']}"
                if opts.get("install_prefix") is not None
                else ""
            )
            c_compiler_arg = str(
                f"-DCMAKE_C_COMPILER={opts['c_compiler']}"
                if opts.get("c_compiler") is not None
                else ""
            )
            cxx_compiler_arg = str(
                f"-DCMAKE_CXX_COMPILER={opts['cxx_compiler']}"
                if opts.get("cxx_compiler") is not None
                else ""
            )
            print(f"Installing dune with prefix argument '{prefix_arg}'")
            opts_file.write(
                f'CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DDUNE_ENABLE_PYTHONBINDINGS=0 {prefix_arg} {c_compiler_arg} {cxx_compiler_arg}"'
            )
        _clone_sources("https://gitlab.dune-project.org/core/dune-common.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/core/dune-geometry.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/core/dune-grid.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/core/dune-localfunctions.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/core/dune-istl.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/extensions/dune-alugrid.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/staging/dune-typetree.git", f"releases/{PACKAGES['dune']}")
        _clone_sources("https://gitlab.dune-project.org/staging/dune-functions.git", f"releases/{PACKAGES['dune']}")
        subprocess.run(["dune-common/bin/dunecontrol", "--opts=dune.opts", "configure"], check=True)
        subprocess.run(["dune-common/bin/dunecontrol", "--opts=dune.opts", "make", "-j4"], check=True)
        subprocess.run(["dune-common/bin/dunecontrol", "--opts=dune.opts", "make", "install"], check=True)
        os.chdir(cwd)
        shutil.rmtree("dune_libs")
    elif name == "mfem":
        _install_from_source({
            "origin": "https://github.com/mfem/mfem.git",
            "branch": f"v{PACKAGES['mfem']}"
        } | opts)
    elif name == "doxygen":
        _install_from_source({
            "origin": "https://github.com/doxygen/doxygen.git",
            "branch": f"{PACKAGES['doxygen']}"
        } | opts)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--pkg-names", required=False, nargs="*")
    parser.add_argument("-a", "--all", required=False, action="store_true")
    parser.add_argument("-p", "--install-prefix", required=False)
    parser.add_argument("-c", "--c-compiler", required=False)
    parser.add_argument("-x", "--cxx-compiler", required=False)
    args = vars(parser.parse_args())

    if not args["pkg_names"] and not args["all"]:
        sys.exit("Either one or more packages or 'all' need to be provided")

    packages = PACKAGES if args["all"] else args["pkg_names"]
    print(f"Installing the following packages: {', '.join(packages)}")
    print("Note that the basic dependencies are assumed to be preinstalled. See 'Dockerfile' for more info.")
    assert all(p in PACKAGES for p in packages)
    for p in packages:
        _install_pkg(p, args)
