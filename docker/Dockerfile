# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

# Dockerfile for the GridFormat GitHub actions
FROM ubuntu:26.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade -y -o Dpkg::Options::="--force-confold" \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM base AS minimal
RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        build-essential \
        cmake \
        # the compilers we test with (oldest gcc, newest clang)
        g++-14 \
        clang-22 \
        python3 \
        python3-pip \
        python3-dev \
        python3-venv \
        wget \
        git \
        apt-utils \
        # for pages job
        rsync \
    && python3 -m venv /gfmt-venv && . /gfmt-venv/bin/activate && pip install vtk==9.7.1 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM minimal AS full
RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        zlib1g \
        liblz4-dev \
        liblzma-dev \
        mpi-default-bin \
        mpi-default-dev \
        libhdf5-mpi-dev \
        build-essential \
        gnupg-agent \
        # for memcheck/docu/coverage
        gcovr \
        valgrind \
        flex \
        bison \
        # CGAL dependencies
        libgmp-dev \
        libmpfr-dev \
        libboost-dev \
        # dolfinx dependencies
        libblas-dev \
        liblapack-dev \
        pybind11-dev \
        libpugixml-dev \
        libboost-timer-dev \
        libboost-filesystem-dev \
        petsc-dev \
        libspdlog-dev \
        # dolfinx requires a graph partitioner (parmetis is not packaged in ubuntu 26.04)
        libscotch-dev \
        libptscotch-dev \
        # fenics wheel builds with ninja
        ninja-build \
        libdeal.ii-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY install_deps_from_sources.py .
RUN . /gfmt-venv/bin/activate \
    # keep the ffcx version in sync with PACKAGES['dolfinx'] in install_deps_from_sources.py
    && pip install fenics-ffcx==0.11.0 pybind11 \
    && python3 install_deps_from_sources.py --all \
                                            --install-prefix=/gfmt-dependencies \
                                            --c-compiler=/usr/bin/gcc-14 \
                                            --cxx-compiler=/usr/bin/g++-14
