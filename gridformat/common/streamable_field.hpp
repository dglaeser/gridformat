// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_
#define GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_

#include <utility>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<std::derived_from<Field> F, Concepts::Encoder<std::ostream> Encoder>
class StreamableField {
 public:
    template<std::convertible_to<const F&> _F> requires(std::is_lvalue_reference_v<_F>)
    StreamableField(_F&& field, Encoder enc)
    : _field(field)
    , _encoder(std::move(enc))
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamableField& field) {
        auto encoded = field._encoder(s);
        auto serialization = field._field.serialized();
        field._field.precision().visit([&] <typename T> (const Precision<T>&) {
            if (serialization.size() != field._field.number_of_entries()*sizeof(T))
                throw SizeError("Serialized size does not match field specifications");

            const auto bytes = serialization.as_span();
            const std::span<const T> values{
                reinterpret_cast<const T*>(bytes.data()),
                field._field.number_of_entries()
            };
            encoded.write(values);
        });
        return s;
    }

 private:
    const F& _field;
    Encoder _encoder;
};

template<typename F, typename Enc>
StreamableField(F&&, Enc&&) -> StreamableField<std::decay_t<F>, std::decay_t<Enc>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_