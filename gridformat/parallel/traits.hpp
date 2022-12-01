// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Parallel
 * \brief Traits for parallel communication.
 */
#ifndef GRIDFORMAT_PARALLEL_TRAITS_HPP_
#define GRIDFORMAT_PARALLEL_TRAITS_HPP_

namespace GridFormat::ParallelTraits {

//! \addtogroup Parallel
//! \{

//! Metafunction to obtain the number of processes from a communicator via a static `int get(const Communicator&)`
template<typename Communicator>
struct Size;

//! Metafunction to obtain the rank of a process from a communicator via a static `int get(const Communicator&)`
template<typename Communicator>
struct Rank;

//! Metafunction to obtain a barrier for all processes to reach before continuation `int get(const Communicator&)`
template<typename Communicator>
struct Barrier;

//! \} group Parallel

}  // namespace GridFormat::ParallelTraits

#if GRIDFORMAT_HAVE_MPI

#include <mpi.h>

namespace GridFormat::ParallelTraits {

template<>
struct Size<MPI_Comm> {
    static int get(MPI_Comm comm) {
        int s;
        MPI_Comm_size(comm, &s);
        return s;
    }
};

template<>
struct Rank<MPI_Comm> {
    static int get(MPI_Comm comm) {
        int r;
        MPI_Comm_rank(comm, &r);
        return r;
    }
};

template<>
struct Barrier<MPI_Comm> {
    static int get(MPI_Comm comm) {
        return MPI_Barrier(comm);
    }
};

}  // namespace GridFormat::ParallelTraits

#endif  // GRIDFORMAT_HAVE_MPI

#endif  // GRIDFORMAT_PARALLEL_TRAITS_HPP_
