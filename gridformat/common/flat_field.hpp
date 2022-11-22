// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::FlatField
 */
#ifndef GRIDFORMAT_COMMON_FLAT_FIELD_HPP_
#define GRIDFORMAT_COMMON_FLAT_FIELD_HPP_

#include <span>
#include <ranges>
#include <utility>
#include <type_traits>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Field implementation around a given range
 */
template<std::ranges::forward_range R, Concepts::Scalar ValueType = MDRangeScalar<R>>
class FlatField : public Field {
    using RangeScalar = MDRangeScalar<R>;

 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit FlatField(_R&& range, const Precision<ValueType>& = {})
    : _range{std::forward<_R>(range)}
    {}

 private:
    template<std::ranges::forward_range _R, std::invocable<RangeScalar> Action>
    void _visit(_R&& range, const Action& action) const {
        if constexpr (has_sub_range<_R>)
            std::ranges::for_each(range, [&] (const std::ranges::range auto& subrange) {
                this->_visit(subrange, action);
            });
        else
            std::ranges::for_each(range, action);
    }

    std::size_t _number_of_entries() const {
        std::size_t count = 0;
        _visit(_range, [&] (const RangeScalar&) { count++; });
        return count;
    }

    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_number_of_entries());
    }

    std::size_t _size_in_bytes(std::size_t num_entries) const {
        return num_entries*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        return MDLayout{{_number_of_entries()}};
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        Serialization serialization(_size_in_bytes());
        auto data = serialization.template as_span_of<ValueType>();
        std::size_t offset = 0;
        _visit(_range, [&] (const RangeScalar& value) {
            data[offset++] = static_cast<ValueType>(value);
        });
        return serialization;
    }

    R _range;
};

template<std::ranges::forward_range R> requires(std::is_lvalue_reference_v<R>)
FlatField(R&& r) -> FlatField<std::remove_reference_t<R>&, MDRangeScalar<std::decay_t<R>>>;
template<std::ranges::forward_range R, Concepts::Scalar T> requires(std::is_lvalue_reference_v<R>)
FlatField(R&& r, const Precision<T>&) -> FlatField<std::remove_reference_t<R>&, T>;

template<std::ranges::forward_range R> requires(!std::is_lvalue_reference_v<R>)
FlatField(R&& r) -> FlatField<std::decay_t<R>>;
template<std::ranges::forward_range R, Concepts::Scalar T> requires(!std::is_lvalue_reference_v<R>)
FlatField(R&& r, const Precision<T>&) -> FlatField<std::decay_t<R>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FLAT_FIELD_HPP_
