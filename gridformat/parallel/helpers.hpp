// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Parallel
 * \brief Helper functions for parallel computations.
 */
#ifndef GRIDFORMAT_PARALLEL_HELPERS_HPP_
#define GRIDFORMAT_PARALLEL_HELPERS_HPP_

#include <array>
#include <ranges>

#include <gridformat/parallel/concepts.hpp>
#include <gridformat/parallel/communication.hpp>

namespace GridFormat::Parallel {

//! \addtogroup Parallel
//! \{

struct Index {
    std::size_t i;
    int rank;
};

//! Access an entry in a vector containing N elements per process for the given rank and index
template<std::size_t N, Concepts::Communicator C, std::ranges::contiguous_range R>
decltype(auto) access_gathered(R&& values, const C& comm, const Index& index) {
    if (std::ranges::size(values) != N*size(comm))
        throw SizeError("Range size does not match number of processors times N");
    return std::ranges::data(std::forward<R>(values))[index.rank*N + index.i];
}

//! Get all entries from a vector containing N elements per process for the given rank
template<std::size_t N, Concepts::Communicator C, std::ranges::contiguous_range R>
auto access_gathered(R&& values, const C& comm, int rank) {
    if (std::ranges::size(values) != N*size(comm))
        throw SizeError("Range size does not match number of processors times N");
    std::array<std::ranges::range_value_t<R>, N> result;
    for (std::size_t i = 0; i < N; ++i)
        result[i] = std::ranges::data(std::forward<R>(values))[rank*N + i];
    return result;
}

//! Return a range over all ranks of the given communicator
template<Concepts::Communicator C>
std::ranges::view auto ranks(const C& comm) {
    return std::views::iota(0, size(comm));
}

//! \} group Parallel

}  // namespace GridFormat::Parallel

#endif  // GRIDFORMAT_PARALLEL_HELPERS_HPP_
