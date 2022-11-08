// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_
#define GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <cmath>

#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

template<std::size_t target_dimension,
         std::ranges::view R> requires(std::ranges::forward_range<R>)
class ExtendedRange {
    using ValueType = std::ranges::range_value_t<R>;

    class Iterator : public ForwardIteratorFacade<Iterator, ValueType, ValueType> {
        using ConstRange = std::add_const_t<std::decay_t<R>>;

     public:
        Iterator() = default;
        explicit Iterator(ConstRange& range, ValueType value, bool is_end = false)
        : _range(&range)
        , _it(is_end ? std::ranges::end(*_range) : std::ranges::begin(*_range))
        , _extension_value(value)
        , _extension_size(_compute_extension_size())
        , _idx_in_extension(is_end ? _extension_size : 0)
        , _is_end(is_end)
        {}

     private:
        friend class IteratorAccess;

        int _compute_extension_size() const {
            assert(
                static_cast<std::size_t>(std::ranges::distance(*_range)) < target_dimension
                && "Provided range is larger than the given target dimension"
            );
            using std::max;
            return max(target_dimension - std::ranges::distance(*_range), std::size_t{0});
        }

        ValueType _dereference() const {
            assert(!_is_end);
            return _in_extension() ? _extension_value : *_it;
        }

        void _increment() {
            if (_in_extension()) {
                if (++_idx_in_extension; _idx_in_extension == _extension_size)
                    _is_end = true;
            } else {
                ++_it;
            }
        }

        bool _is_equal(const Iterator& other) const {
            if (_range != other._range)
                return false;
            if (_extension_value != other._extension_value)
                return false;
            if (_is_end == other._is_end)
                return true;
            return _it == other._it && _idx_in_extension == other._idx_in_extension;
        }

        bool _in_extension() const {
            return _it == std::ranges::end(*_range);
        }

        ConstRange* _range;
        std::ranges::iterator_t<ConstRange> _it;
        ValueType _extension_value;
        int _extension_size;
        int _idx_in_extension;
        bool _is_end;
    };

 public:
    explicit ExtendedRange(R&& range, ValueType value)
    : _range(std::move(range))
    , _value(value)
    {}

    auto begin() const { return Iterator{_range, _value}; }
    auto end() const { return Iterator{_range, _value, true}; }

 private:
    R _range;
    ValueType _value;
};

template<std::size_t target_dimension, std::ranges::range R>
constexpr auto make_extended(R&& range, const std::ranges::range_value_t<R>& value = {0.0}) {
    return ExtendedRange<target_dimension, std::decay_t<R>>{std::forward<R>(range), value};
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_