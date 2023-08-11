// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Parallel
 * \brief Concepts for parallel communicators.
 */
#ifndef GRIDFORMAT_PARALLEL_CONCEPTS_HPP_
#define GRIDFORMAT_PARALLEL_CONCEPTS_HPP_

#include <concepts>
#include <type_traits>

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
    = is_complete<ParallelTraits::Size<std::remove_cvref_t<T>>>
    and is_complete<ParallelTraits::Rank<std::remove_cvref_t<T>>>
    and is_complete<ParallelTraits::Barrier<std::remove_cvref_t<T>>>
    and requires(const T& t) {
        { ParallelTraits::Size<std::remove_cvref_t<T>>::get(t) } -> std::convertible_to<int>;
        { ParallelTraits::Rank<std::remove_cvref_t<T>>::get(t) } -> std::convertible_to<int>;
        { ParallelTraits::Barrier<std::remove_cvref_t<T>>::get(t) } -> std::convertible_to<int>;
    };

template<typename T>
concept MaxCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Max<T>>;

template<typename T>
concept MinCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Min<T>>;

template<typename T>
concept SumCommunicator = ParallelDetail::Reducer<T, ParallelTraits::Sum<T>>;

template<typename T>
concept BroadCastCommunicator = is_complete<ParallelTraits::BroadCast<T>> && requires(const T& t) {
    { ParallelTraits::BroadCast<T>::get(t, int{}) } -> std::convertible_to<int>;
    { ParallelTraits::BroadCast<T>::get(t, double{}) } -> std::convertible_to<double>;
    { ParallelTraits::BroadCast<T>::get(t, std::array<int, 2>{}) } -> RangeOf<int>;
    { ParallelTraits::BroadCast<T>::get(t, std::array<double, 2>{}) } -> RangeOf<double>;
};

template<typename T>
concept GatherCommunicator = is_complete<ParallelTraits::Gather<T>> && requires(const T& t) {
    { ParallelTraits::Gather<T>::get(t, int{}) } -> RangeOf<int>;
    { ParallelTraits::Gather<T>::get(t, double{}) } -> RangeOf<double>;
    { ParallelTraits::Gather<T>::get(t, std::array<int, 2>{}) } -> RangeOf<int>;
    { ParallelTraits::Gather<T>::get(t, std::array<double, 2>{}) } -> RangeOf<double>;
};

template<typename T>
concept ScatterCommunicator = is_complete<ParallelTraits::Scatter<T>> && requires(const T& t) {
    { ParallelTraits::Scatter<T>::get(t, std::array<int, 2>{}) } -> RangeOf<int>;
    { ParallelTraits::Scatter<T>::get(t, std::array<double, 2>{}) } -> RangeOf<double>;
};

//! \} group Concepts

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_PARALLEL_CONCEPTS_HPP_
