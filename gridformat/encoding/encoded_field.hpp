// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Wraps a field and makes it streamable using encoding
 */
#ifndef GRIDFORMAT_ENCODING_ENCODED_FIELD_HPP_
#define GRIDFORMAT_ENCODING_ENCODED_FIELD_HPP_

#include <utility>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/encoding/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Encoding
 * \brief Wraps a field and makes it streamable using encoding
 */
template<std::derived_from<Field> F, Concepts::Encoder<std::ostream> Encoder>
class EncodedField {
 public:
    template<std::convertible_to<const F&> _F>
        requires(std::is_lvalue_reference_v<_F>)
    EncodedField(_F&& field, Encoder enc)
    : _field(field)
    , _encoder(std::move(enc))
    {}

    friend std::ostream& operator<<(std::ostream& s, const EncodedField& field) {
        auto encoded = field._encoder(s);
        field._field.visit_field_values([&] <typename T> (std::span<const T> data) {
            encoded.write(data);
        });
        return s;
    }

 private:
    const F& _field;
    Encoder _encoder;
};

template<typename F, typename Enc>
EncodedField(F&&, Enc&&) -> EncodedField<std::remove_cvref_t<F>, std::remove_cvref_t<Enc>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_ENCODING_ENCODED_FIELD_HPP_
