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

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/type_traits.hpp>

#include <gridformat/parallel/traits.hpp>

namespace GridFormat::Concepts {

#ifndef DOXYGEN
namespace ParallelDetail {

    template<typename T, typename R>
    concept Reducer = is_complete<R> && requires(const T& t) {
        { R::get(t, int{}) } -> std::convertible_to<int>;
        { R::get(t, double{}) } -> std::convertible_to<double>;
        { R::get(t, std::array<int, 2>{}) } -> RangeOf<int>;
        { R::get(t, std::array<double, 2>{}) } -> RangeOf<double>;
    };

}  // namespace ParallelDetail
#endif  // DOXYGEN

//! \addtogroup Concepts
//! \{

template<typename T>
concept Communicator
    = is_complete<ParallelTraits::Size<T>>
    and is_complete<ParallelTraits::Rank<T>>
    and is_complete<ParallelTraits::Barrier<T>>
    and requires(const T& t) {
        { ParallelTraits::Size<T>::get(t) } -> std::convertible_to<int>;
        { ParallelTraits::Rank<T>::get(t) } -> std::convertible_to<int>;
        { ParallelTraits::Barrier<T>::get(t) } -> std::convertible_to<int>;
    };

template<typename T>
concept MaxCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Max<T>>;

template<typename T>
concept MinCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Min<T>>;

template<typename T>
concept SumCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Sum<T>>;

//! \} group Concepts

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_PARALLEL_CONCEPTS_HPP_
