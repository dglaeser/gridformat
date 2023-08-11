// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for ranges
 */
#ifndef GRIDFORMAT_COMMON_RANGES_HPP_
#define GRIDFORMAT_COMMON_RANGES_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <iterator>
#include <concepts>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <optional>
#include <sstream>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat::Ranges {

/*!
 * \ingroup Common
 * \brief Return the size of a range
 */
template<std::ranges::sized_range R>
    requires(!Concepts::StaticallySizedRange<R>)
inline constexpr auto size(R&& r) {
    return std::ranges::size(r);
}

/*!
 * \ingroup Common
 * \brief Return the size of a range
 * \note This has complexitx O(N), but we also want to support user-given non-sized ranges.
 */
template<std::ranges::range R>
    requires(!std::ranges::sized_range<R> and
             !Concepts::StaticallySizedRange<R>)
inline constexpr auto size(R&& r) {
    return std::ranges::distance(r);
}

/*!
 * \ingroup Common
 * \brief Return the size of a range with size known at compile time.
 */
template<Concepts::StaticallySizedRange R>
inline constexpr auto size(R&&) {
    return static_size<R>;
}

/*!
 * \ingroup Common
 * \brief Return the value at the i-th position of the range.
 */
template<std::integral I, std::ranges::range R>
inline constexpr decltype(auto) at(I i, const R& r) {
    auto it = std::ranges::begin(r);
    std::advance(it, i);
    return *it;
}

/*!
 * \ingroup Common
 * \brief Return an array of the desired size, filled with the given value.
 */
template<std::size_t dim, typename T>
inline constexpr auto filled_array(const T& t = default_value<T>) {
    std::array<T, dim> result;
    std::ranges::fill(result, t);
    return result;
}

/*!
 * \ingroup Common
 * \brief Return an array containing the result of an operation
 *        applied to pairs of elements of the two given ranges.
 */
template<typename TargetType = Automatic,
         typename Op,
         Concepts::StaticallySizedRange R1,
         Concepts::StaticallySizedRange R2>
    requires(static_size<R1> == static_size<R2> and
             std::invocable<Op, std::ranges::range_reference_t<R1>, std::ranges::range_reference_t<R2>>)
inline constexpr auto apply_pairwise(const Op& op, R1&& r1, R2&& r2) {
    using RR1 = std::ranges::range_reference_t<R1>;
    using RR2 = std::ranges::range_reference_t<R2>;
    using T = std::conditional_t<
        std::is_same_v<TargetType, Automatic>,
        std::invoke_result_t<Op, RR1, RR2>,
        TargetType
    >;
    auto result = filled_array<static_size<R1>, T>();
    std::transform(
        std::ranges::begin(r1),
        std::ranges::end(r1),
        std::ranges::begin(r2),
        result.begin(),
        op
    );
    return result;
}

#ifndef DOXYGEN
namespace Detail {

    template<auto N, typename R>
    struct ResultArraySize;

    template<Concepts::StaticallySizedRange R>
    struct ResultArraySize<automatic, R> : std::integral_constant<std::size_t, static_size<R>> {};

    template<std::integral auto n, typename R>
    struct ResultArraySize<n, R> : std::integral_constant<std::size_t, n> {};

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Convert the given range into an array with the given dimension.
 */
template<auto n = automatic, typename T = Automatic, std::ranges::range R>
inline constexpr auto to_array(const R& r) {
    using N = std::remove_cvref_t<decltype(n)>;
    static_assert(std::integral<N> || std::same_as<N, Automatic>);
    static_assert(Concepts::StaticallySizedRange<R> || !std::same_as<N, Automatic>);
    constexpr std::size_t result_size = Detail::ResultArraySize<n, R>::value;
    const std::size_t range_size = size(r);

    using ValueType = std::conditional_t<std::is_same_v<T, Automatic>, std::ranges::range_value_t<R>, T>;
    std::array<ValueType, result_size> result;
    std::ranges::copy_n(std::ranges::begin(r), std::min(range_size, result_size), result.begin());
    if (range_size < result_size)
        std::ranges::fill_n(result.begin() + range_size, result_size - range_size, ValueType{0});
    return result;
}

/*!
 * \ingroup Common
 * \brief Create an array of given type and size from a string.
 */
template<typename T, std::size_t N>
std::array<T, N> array_from_string(const std::string& values) {
    auto result = Ranges::filled_array<N>(T{0});
    std::stringstream stream;
    stream << values;
    auto stream_view = std::ranges::istream_view<T>(stream);
    const auto [_, out] = std::ranges::copy(stream_view, result.begin());
    if (std::ranges::distance(result.begin(), out) != N)
        throw IOError("Could not read " + std::to_string(N) + " values from '" + values + "'");
    return result;
}

/*!
 * \ingroup Common
 * \brief Return a copy of the range with each entry incremented by the given value.
 */
template<std::ranges::range R, typename Increment>
inline constexpr auto incremented(R&& r, const Increment& inc) {
    auto result = std::forward<R>(r);
    std::ranges::for_each(result, [&] (auto& value) { value += inc; });
    return result;
}

/*!
 * \ingroup Common
 * \brief Return a range that contains the elements of the two given ranges.
 */
template<Concepts::StaticallySizedRange R1, Concepts::StaticallySizedRange R2>
    requires(std::is_same_v<std::ranges::range_value_t<R1>, std::ranges::range_value_t<R2>>)
inline constexpr auto merged(R1&& r1, R2&& r2) {
    constexpr std::size_t merged_size = static_size<R1> + static_size<R2>;
    auto result = filled_array<merged_size>(*std::ranges::begin(r1));
    std::ranges::copy(r1, result.begin());
    std::ranges::copy(r2, result.begin() + static_size<R1>);
    return result;
}

/*!
 * \ingroup Common
 * \brief Flatten the given 2d range into a 1d range.
 */
template<Concepts::MDRange<2> R>
    requires(Concepts::StaticallySizedMDRange<std::ranges::range_value_t<R>, 1>)
inline constexpr auto flat(const R& r) {
    if constexpr (Concepts::StaticallySizedRange<R>) {
        constexpr std::size_t element_size = static_size<std::ranges::range_value_t<R>>;
        constexpr std::size_t flat_size = element_size*static_size<R>;
        std::array<MDRangeValueType<R>, flat_size> result;
        auto it = result.begin();
        std::ranges::for_each(r, [&] (const auto& sub_range) {
            std::ranges::for_each(sub_range, [&] (const auto& entry) {
                *it = entry;
                ++it;
            });
        });
        return result;
    } else {
        std::vector<MDRangeValueType<R>> result;
        result.reserve(size(r)*static_size<std::ranges::range_value_t<R>>);
        std::ranges::for_each(r, [&] (const auto& sub_range) {
            std::ranges::for_each(sub_range, [&] (const auto& entry) {
                result.push_back(entry);
            });
        });
        return result;
    }
}

/*!
 * \ingroup Common
 * \brief Sort the given range and remove all duplicates.
 */
template<std::ranges::range R,
         typename Comp = std::ranges::less,
         typename EqPredicate = std::equal_to<std::ranges::range_value_t<R>>>
inline constexpr decltype(auto) sort_and_unique(R&& r,
                                                Comp comparator = {},
                                                EqPredicate eq = {}) {
    std::ranges::sort(r, comparator);
    return std::ranges::unique(std::forward<R>(r), eq);
}

/*!
 * \ingroup Common
 * \brief Adapter to expose a multi-dimensional range as a flat range.
 */
template<std::ranges::forward_range Range>
class FlatView : public std::ranges::view_interface<FlatView<Range>> {
    static constexpr std::size_t dim = mdrange_dimension<Range>;
    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<Range>>;

    using ValueType = GridFormat::MDRangeValueType<Range>;
    using ReferenceType = GridFormat::MDRangeReferenceType<Range>;

    template<std::ranges::forward_range R>
    class Iterator;

    template<std::ranges::forward_range R> requires(mdrange_dimension<R> == 1)
    class Iterator<R> : public ForwardIteratorFacade<Iterator<R>, ValueType, ReferenceType> {
     public:
        Iterator() = default;
        Iterator(std::ranges::iterator_t<R> begin, std::ranges::iterator_t<R> end)
        : _it{begin}
        , _end{end}
        {}

     private:
        friend class GridFormat::IteratorAccess;
        ReferenceType _dereference() const { return *_it; }
        bool _is_equal(const Iterator& other) const { return _it == other._it; }
        void _increment() { ++_it; }

        std::ranges::iterator_t<R> _it;
        std::ranges::iterator_t<R> _end;
    };

    template<std::ranges::forward_range R> requires(mdrange_dimension<R> > 1)
    class Iterator<R> : public ForwardIteratorFacade<Iterator<R>, ValueType, ReferenceType> {
        using RangeValueType = std::remove_reference_t<std::ranges::range_reference_t<R>>;
        using SubIterator = Iterator<RangeValueType>;

     public:
        Iterator() = default;
        Iterator(std::ranges::iterator_t<R> begin, std::ranges::iterator_t<R> end)
        : _it{begin}
        , _end{end} {
            if (_it != _end)
                _make_sub_iterators();
        }

    // TODO: gcc does not take this friend declaration!?
    //       These interfaces should be private :/
    //  private:
    //     friend class GridFormat::IteratorAccess;

        bool _is_sub_end() const {
            return !static_cast<bool>(_sub_it);
        }

        ReferenceType _dereference() const {
            assert(!_is_sub_end());
            return *(*_sub_it);
        }

        bool _is_equal(const Iterator& other) const {
            if (_it != other._it)
                return false;
            else if (!_is_sub_end() && !other._is_sub_end())
                return *_sub_it == *other._sub_it;
            return true;
        }

        void _increment() {
            if (_is_sub_end() || _it == _end)
                throw InvalidState("Cannot increment past end iterator");

            if (++(*_sub_it); *_sub_it == *_sub_end) {
                if (++_it; _it != _end)
                    _make_sub_iterators();
                else
                    _release_sub_iterators();
            }
        }

     private:
        void _make_sub_iterators() {
            _sub_it = SubIterator{std::ranges::begin(*_it), std::ranges::end(*_it)};
            _sub_end = SubIterator{std::ranges::end(*_it), std::ranges::end(*_it)};
        }

        void _release_sub_iterators() {
            _sub_it.reset();
            _sub_end.reset();
        }

        std::ranges::iterator_t<R> _it;
        std::ranges::iterator_t<R> _end;
        std::optional<SubIterator> _sub_it;
        std::optional<SubIterator> _sub_end;
    };

 public:
    explicit FlatView(Range& r)
    : _range(r)
    {}

    Iterator<Range> begin() requires(!is_const) {
        return {std::ranges::begin(_range.get()), std::ranges::end(_range.get())};
    }

    Iterator<std::add_const_t<Range>> begin() const {
        return {std::ranges::begin(_range.get()), std::ranges::end(_range.get())};
    }

    Iterator<Range> end() requires(!is_const) {
        return {std::ranges::end(_range.get()), std::ranges::end(_range.get())};
    }

    Iterator<std::add_const_t<Range>> end() const {
        return {std::ranges::end(_range.get()), std::ranges::end(_range.get())};
    }

 private:
    std::reference_wrapper<Range> _range;
};

}  // namespace GridFormat::Ranges

namespace GridFormat::Views {

#ifndef DOXYGEN
namespace Detail {

    struct FlatViewAdapter {
        template<std::ranges::forward_range R> requires(std::is_lvalue_reference_v<R>)
        constexpr auto operator()(R&& in) const {
            return Ranges::FlatView{std::forward<R>(in)};
        }
    };

    template<std::ranges::forward_range R> requires(std::ranges::viewable_range<R>)
    inline constexpr std::ranges::view auto operator|(R&& range, const FlatViewAdapter& adapter) {
        return adapter(std::forward<R>(range));
    }

}  // namespace Detail
#endif  // DOXYGEN

inline constexpr Detail::FlatViewAdapter flat;

}  // namespace GridFormat::Views


// Disable the size semantics for our flat view because checking
// the sized_range concept for it results in a problem with recursion
namespace std::ranges {

template<typename R>
inline constexpr bool disable_sized_range<GridFormat::Ranges::FlatView<R>> = true;

}  // namespace std::ranges

#endif  // GRIDFORMAT_COMMON_RANGES_HPP_
