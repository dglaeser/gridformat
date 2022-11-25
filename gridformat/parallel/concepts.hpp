// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Parallel
 * \brief Concepts for parallel communicators.
 */
#ifndef GRIDFORMAT_PARALLEL_CONCEPTS_HPP_
#define GRIDFORMAT_PARALLEL_CONCEPTS_HPP_

#include <concepts>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/parallel/traits.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Concepts
//! \{

template<typename T>
concept Communicator
    = is_complete<ParallelTraits::Size<T>>
    and is_complete<ParallelTraits::Rank<T>>
    and requires(const T& t) {
        { ParallelTraits::Size<T>::get(t) } -> std::convertible_to<int>;
        { ParallelTraits::Rank<T>::get(t) } -> std::convertible_to<int>;
    };

//! \} group Concepts

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_PARALLEL_CONCEPTS_HPP_
