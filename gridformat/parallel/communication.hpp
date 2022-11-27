// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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

//! \} group GrParallelid

}  // namespace GridFormat::Parallel

#endif  // GRIDFORMAT_PARALLEL_COMMUNICATION_HPP_
