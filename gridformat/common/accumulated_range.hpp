// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::AccumulatedRange
 */
#ifndef GRIDFORMAT_COMMON_ACCUMULATED_RANGE_HPP_
#define GRIDFORMAT_COMMON_ACCUMULATED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <concepts>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Turns a range of integers into a range over their sums
 */
template<Concepts::MDRange<1> R>
requires(std::integral<MDRangeValueType<R>> and std::ranges::forward_range<R>)
class AccumulatedRange {
    class Iterator : public ForwardIteratorFacade<Iterator, std::size_t, const std::size_t&> {
        using ConstRange = std::add_const_t<std::decay_t<R>>;

     public:
        Iterator() = default;
        explicit Iterator(ConstRange& range, bool is_end = false)
        : _range{&range}
        , _it{is_end ? std::ranges::end(*_range) : std::ranges::begin(*_range)}
        , _end_it{std::ranges::end(*_range)}
        , _count{0} {
            if (!is_end)
                _count += *std::ranges::begin(*_range);
        }

     private:
        friend class IteratorAccess;

        const std::size_t& _dereference() const {
            assert(!_is_end());
            return _count;
        }

        void _increment() {
            assert(!_is_end());
            if (++_it; !_is_end())
                _count += *_it;
            else
                _count = 0;
        }

        bool _is_equal(const Iterator& other) const {
            if (_range != other._range)
                return false;
            return _it == other._it && _count == other._count;
        }

        bool _is_end() const {
            return _it == _end_it;
        }

        ConstRange* _range;
        std::ranges::iterator_t<ConstRange> _it;
        std::ranges::iterator_t<ConstRange> _end_it;
        std::size_t _count;
    };

 public:
    template<std::ranges::range _R> requires(std::convertible_to<_R, R>)
    explicit AccumulatedRange(_R&& range)
    : _range{std::forward<_R>(range)}
    {}

    auto begin() const { return Iterator{_range}; }
    auto end() const { return Iterator{_range, true}; }

 private:
    R _range;
};

template<std::ranges::range R> requires(!std::is_lvalue_reference_v<R>)
AccumulatedRange(R&& r) -> AccumulatedRange<std::decay_t<R>>;
template<std::ranges::range R> requires(std::is_lvalue_reference_v<R>)
AccumulatedRange(R&& r) -> AccumulatedRange<R&>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_COUNTED_RANGE_HPP_