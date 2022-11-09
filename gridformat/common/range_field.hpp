// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_RANGE_FIELD_HPP_
#define GRIDFORMAT_COMMON_RANGE_FIELD_HPP_

#include <utility>
#include <ranges>
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
 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit RangeField(_R&& range, const Precision<ValueType>& prec = {})
    : Field(get_layout(range), prec)
    , _range(std::forward<_R>(range))
    {}

 private:
    void _stream(FormattedAsciiOutputStream& stream) const override {
        _visit(_range, [&] (const ValueType& value) {
            stream << value;
        });
    }

    typename Field::Serialization _serialized() const override {
        typename Field::Serialization serialization{this->layout().number_of_entries()*sizeof(ValueType)};
        ValueType* data = reinterpret_cast<ValueType*>(serialization.data());
        std::size_t offset = 0;
        _visit(_range, [&] (const ValueType& value) {
            data[offset++] = value;
        });
        return serialization;
    }

    template<typename Action>
    void _visit(const std::ranges::range auto& r, const Action& action) const {
        std::ranges::for_each(r, [&] (const auto& entry) {
            _visit(entry, action);
        });
    }

    template<typename Action>
    void _visit(const Concepts::Scalar auto& value, const Action& action) const {
        action(static_cast<ValueType>(value));
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