// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Parallel
 * \brief Interface for parallel communication.
 */
#ifndef GRIDFORMAT_PARALLEL_COMMUNICATION_HPP_
#define GRIDFORMAT_PARALLEL_COMMUNICATION_HPP_

#include <gridformat/parallel/traits.hpp>
#include <gridformat/parallel/concepts.hpp>

namespace GridFormat::Parallel {

//! \addtogroup Parallel
//! \{

//! Return the number of processes in a communication
template<Concepts::Communicator C>
inline int size(const C& comm) {
    return ParallelTraits::Size<C>::get(comm);
}

//! Return the rank of a processes in a communication
template<Concepts::Communicator C>
inline int rank(const C& comm) {
    return ParallelTraits::Rank<C>::get(comm);
}

//! Return a barrier
template<Concepts::Communicator C>
inline int barrier(const C& comm) {
    return ParallelTraits::Barrier<C>::get(comm);
}

//! Return the maximum of the given values over all processes
template<Concepts::MaxCommunicator C, typename T>
inline auto max(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::Max<C>::get(comm, values, root);
}

//! Return the minimum of the given values over all processes
template<Concepts::MinCommunicator C, typename T>
inline auto min(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::Min<C>::get(comm, values, root);
}

//! Return the sum of the given values over all processes
template<Concepts::SumCommunicator C, typename T>
inline auto sum(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::Sum<C>::get(comm, values, root);
}

//! Broadcast values from the root to all other processes
template<Concepts::SumCommunicator C, typename T>
inline auto broadcast(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::BroadCast<C>::get(comm, values, root);
}

//! Gather values from all processes to the root process
template<Concepts::SumCommunicator C, typename T>
inline auto gather(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::Gather<C>::get(comm, values, root);
}

//! Scatter values from the root to all other processes
template<Concepts::SumCommunicator C, typename T>
inline auto scatter(const C& comm, const T& values, int root = 0) {
    return ParallelTraits::Scatter<C>::get(comm, values, root);
}

//! \} group Parallel

}  // namespace GridFormat::Parallel

#endif  // GRIDFORMAT_PARALLEL_COMMUNICATION_HPP_
