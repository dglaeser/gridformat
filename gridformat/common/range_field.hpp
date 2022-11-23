// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::RangeField
 */
#ifndef GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
#define GRIDFORMAT_COMMON_RANGE_FIELD_HPP_

#include <span>
#include <ranges>
#include <utility>
#include <algorithm>
#include <type_traits>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Field implementation around a given range.
 *        Exposes the range as an m-dimensional field of values.
 * \note This requires the sub-ranges of multi-dimensional ranges
 *       to all have the same extents, as otherwise a `MDLayout`
 *       cannot be properly defined.
 */
template<std::ranges::forward_range R, Concepts::Scalar ValueType = MDRangeScalar<R>>
class RangeField : public Field {
    static constexpr bool is_contiguous_scalar_range =
        Concepts::MDRange<std::decay_t<R>, 1> and
        std::ranges::contiguous_range<std::decay_t<R>> and
        std::ranges::sized_range<std::decay_t<R>>;

    static constexpr bool use_range_value_type =
        std::is_same_v<std::ranges::range_value_t<std::decay_t<R>>, ValueType>;

    static constexpr bool is_contiguous_result_range =
        is_contiguous_scalar_range && use_range_value_type;

 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit RangeField(_R&& range, const Precision<ValueType>& = {})
    : _range{std::forward<_R>(range)}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        return get_md_layout(_range);
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        const auto layout = get_md_layout(_range);
        Serialization serialization(_size_in_bytes(layout));
        _fill(serialization);
        return serialization;
    }

    void _fill(Serialization& serialization) const requires(is_contiguous_result_range) {
        const auto layout = get_md_layout(_range);
        const auto data = std::as_bytes(std::span{_range});
        if (data.size() != _size_in_bytes(layout))
            throw SizeError("Range size does not match the expected serialized size");
        std::ranges::copy(data, serialization.as_span().begin());
    }

    void _fill(Serialization& serialization) const requires(!is_contiguous_result_range) {
        std::size_t offset = 0;
        _fill_buffer(_range, serialization, offset);
    }

    void _fill_buffer(const std::ranges::range auto& r,
                      Serialization& serialization,
                      std::size_t& offset) const {
        std::ranges::for_each(r, [&] (const auto& entry) {
            _fill_buffer(entry, serialization, offset);
        });
    }

    void _fill_buffer(const Concepts::Scalar auto& value,
                      Serialization& serialization,
                      std::size_t& offset) const {
        const auto cast_value = static_cast<ValueType>(value);
        std::ranges::copy(
            std::as_bytes(std::span{&cast_value, 1}),
            serialization.as_span().data() + offset
        );
        offset += sizeof(ValueType);
    }

    R _range;
};

template<std::ranges::forward_range R> requires(std::is_lvalue_reference_v<R>)
RangeField(R&& r) -> RangeField<std::remove_reference_t<R>&, MDRangeScalar<std::decay_t<R>>>;
template<std::ranges::forward_range R, Concepts::Scalar T> requires(std::is_lvalue_reference_v<R>)
RangeField(R&& r, const Precision<T>&) -> RangeField<std::remove_reference_t<R>&, T>;

template<std::ranges::forward_range R> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&& r) -> RangeField<std::decay_t<R>>;
template<std::ranges::forward_range R, Concepts::Scalar T> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&& r, const Precision<T>&) -> RangeField<std::decay_t<R>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
