// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
#define GRIDFORMAT_COMMON_RANGE_FIELD_HPP_

#include <span>
#include <ranges>
#include <utility>
#include <type_traits>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
template<Concepts::FieldValuesRange R, Concepts::Scalar ValueType = MDRangeScalar<R>>
class RangeField : public Field {
    static constexpr bool is_contiguous_scalar_range =
        Concepts::MDRange<std::decay_t<R>, 1> and
        std::ranges::contiguous_range<std::decay_t<R>> and
        std::ranges::sized_range<std::decay_t<R>>;

    static constexpr bool use_range_value_type =
        std::is_same_v<std::ranges::range_value_t<std::decay_t<R>>, ValueType>;

    static constexpr bool skip_cast = is_contiguous_scalar_range && use_range_value_type;

 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit RangeField(_R&& range, const Precision<ValueType>& prec = {})
    : Field(get_layout(range), prec)
    , _range(std::forward<_R>(range))
    {}

 private:
    void _visit(FieldVisitor& visitor) const override {
        if constexpr (skip_cast)
            _visit_without_cast(visitor);
        else
            _visit_with_cast(visitor);
    }

    void _visit_without_cast(FieldVisitor& visitor) const requires(skip_cast) {
        const auto range_size = std::ranges::size(_range);
        const auto* data = reinterpret_cast<const std::byte*>(std::ranges::data(_range));
        visitor.take_field_values(this->precision(), data, range_size*sizeof(ValueType));
    }

    void _visit_with_cast(FieldVisitor& visitor) const requires(!skip_cast) {
        std::vector<ValueType> data(this->layout().number_of_entries());
        std::size_t offset = 0;
        _fill_buffer(_range, data, offset);
        const std::byte* byte_ptr = reinterpret_cast<const std::byte*>(data.data());
        visitor.take_field_values(this->precision(),byte_ptr, data.size()*sizeof(ValueType));
    }

    void _fill_buffer(const std::ranges::range auto& r,
                      std::vector<ValueType>& data,
                      std::size_t& offset) const {
        std::ranges::for_each(r, [&] (const auto& entry) {
            _fill_buffer(entry, data, offset);
        });
    }

    void _fill_buffer(const Concepts::Scalar auto& value,
                      std::vector<ValueType>& data,
                      std::size_t& offset) const {
        data[offset++] = static_cast<ValueType>(value);
    }

    R _range;
};

template<Concepts::FieldValuesRange R> requires(std::is_lvalue_reference_v<R>)
RangeField(R&& r) -> RangeField<const std::decay_t<R>&, MDRangeScalar<std::decay_t<R>>>;
template<Concepts::FieldValuesRange R, Concepts::Scalar T> requires(std::is_lvalue_reference_v<R>)
RangeField(R&& r, const Precision<T>&) -> RangeField<const std::decay_t<R>&, T>;

template<Concepts::FieldValuesRange R> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&& r) -> RangeField<std::decay_t<R>>;
template<Concepts::FieldValuesRange R, Concepts::Scalar T> requires(!std::is_lvalue_reference_v<R>)
RangeField(R&& r, const Precision<T>&) -> RangeField<std::decay_t<R>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RANGE_FIELD_HPP_