// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::ScalarField
 */
#ifndef GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_
#define GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_

#include <utility>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

template<Concepts::Scalar T, Concepts::Scalar ValueType = T>
class ScalarField : public Field {
    static constexpr bool do_cast = !std::is_same_v<T, ValueType>;

 public:
    explicit ScalarField(T value, const Precision<ValueType> = {})
    : _value{std::move(value)}
    {}

 private:
    MDLayout _layout() const { return MDLayout{{1}}; }
    DynamicPrecision _precision() const { return {Precision<ValueType>{}}; }
    Serialization _serialized() const { return Serialization::from_scalar(_get_value()); }

    ValueType _get_value() const requires(!do_cast) { return _value; }
    ValueType _get_value() const requires(do_cast) { return static_cast<ValueType>(_value); }

    T _value;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_
