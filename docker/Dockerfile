# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

# Dockerfile for the GridFormat GitHub actions
FROM ubuntu:24.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade -y -o Dpkg::Options::="--force-confold" \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM base as minimal
RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        build-essential \
        cmake \
        g++-12 g++-14 \
        clang-16 clang-18 \
        python3 \
        python3-pip \
        python3-dev \
        python3-venv \
        wget \
        git \
        apt-utils \
        # for pages job
        rsync \
        # for deal.ii
        software-properties-common \
    && python3 -m venv /gfmt-venv && . /gfmt-venv/bin/activate && pip install vtk \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM minimal as full
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
        # fenics wheel builds with ninja
        ninja-build \
    # deal.ii
    && add-apt-repository --yes ppa:ginggs/deal.ii-9.6.0-backports \
    && apt-get install --yes libdeal.ii-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY install_deps_from_sources.py .
RUN . /gfmt-venv/bin/activate \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/gcc-12 100 \
    && pip install fenics-ffcx==0.6.0 pybind11 \
    && python3 install_deps_from_sources.py --all \
                                            --install-prefix=/gfmt-dependencies \
                                            --c-compiler=/usr/bin/gcc-12 \
                                            --cxx-compiler=/usr/bin/g++-12
