// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::ExtendedRange
 */
#ifndef GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_
#define GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <cmath>
#include <concepts>
#include <type_traits>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Extends a given range up to the given target
 *        dimension by apppending the given value.
 */
template<std::ranges::forward_range R>
class ExtendedRange {
    using ValueType = std::ranges::range_value_t<R>;

    class Iterator : public ForwardIteratorFacade<Iterator, ValueType, ValueType> {
        using ConstRange = std::add_const_t<std::decay_t<R>>;

     public:
        Iterator() = default;
        explicit Iterator(const ExtendedRange& ext, bool is_end = false)
        : _ext_range{&ext}
        , _it{
            is_end ? std::ranges::end(_ext_range->_range)
                   : std::ranges::begin(_ext_range->_range)
        }
        , _idx_in_extension{is_end ? _ext_range->_extension_size : 0}
        , _is_end{is_end}
        {}

     private:
        friend class IteratorAccess;

        ValueType _dereference() const {
            assert(!_is_end);
            return _in_extension() ? _ext_range->_value : *_it;
        }

        void _increment() {
            if (_in_extension()) {
                if (++_idx_in_extension; _idx_in_extension == _ext_range->_extension_size)
                    _is_end = true;
            } else {
                ++_it;
            }
        }

        bool _is_equal(const Iterator& other) const {
            if (_ext_range != other._ext_range)
                return false;
            if (_is_end == other._is_end)
                return true;
            return _it == other._it && _idx_in_extension == other._idx_in_extension;
        }

        bool _in_extension() const {
            return _it == std::ranges::end(_ext_range->_range);
        }

        const ExtendedRange* _ext_range;
        std::ranges::iterator_t<std::add_const_t<R>> _it;
        std::size_t _idx_in_extension;
        bool _is_end;
    };

 public:
    template<std::ranges::range _R> requires(std::convertible_to<_R, R>)
    explicit ExtendedRange(_R&& range, std::size_t extension_size, ValueType value = {})
    : _range{std::forward<_R>(range)}
    , _value{value}
    , _extension_size(extension_size)
    {}

    auto begin() const { return Iterator{*this,}; }
    auto end() const { return Iterator{*this, true}; }

 private:
    R _range;
    ValueType _value;
    std::size_t _extension_size;
};

template<std::ranges::range R> requires(!std::is_lvalue_reference_v<R>)
ExtendedRange(R&&, std::size_t, std::ranges::range_value_t<R> = {}) -> ExtendedRange<std::decay_t<R>>;
template<std::ranges::range R> requires(std::is_lvalue_reference_v<R>)
ExtendedRange(R&&, std::size_t, std::ranges::range_value_t<R> = {}) -> ExtendedRange<std::remove_reference_t<R>&>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_EXTENDED_RANGE_HPP_
