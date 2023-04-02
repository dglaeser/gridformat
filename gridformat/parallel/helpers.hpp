// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Parallel
 * \brief Helper functions for parallel computations.
 */
#ifndef GRIDFORMAT_PARALLEL_HELPERS_HPP_
#define GRIDFORMAT_PARALLEL_HELPERS_HPP_

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

//! Access a vector containing N elements per process for the given rank and index
template<std::size_t N, Concepts::Communicator C, std::ranges::contiguous_range R>
decltype(auto) access_gathered(R&& values, const C& comm, const Index& index) {
    if (std::ranges::size(values) != N*size(comm))
        throw SizeError("Range size does not match number of processors times N");
    return std::ranges::data(std::forward<R>(values))[index.rank*N + index.i];
}

//! \} group Parallel

}  // namespace GridFormat::Parallel

#endif  // GRIDFORMAT_PARALLEL_HELPERS_HPP_
