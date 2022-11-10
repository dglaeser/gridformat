// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_
#define GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_

#include <memory>
#include <utility>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/range_formatter.hpp>
#include <gridformat/common/streams.hpp>

namespace GridFormat {

struct NullEncoder {
    std::ostream& operator()(std::ostream& s) const {
        return s;
    }
};

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<std::derived_from<Field> F,
         typename Encoder = NullEncoder>
class StreamableField {
 public:
    StreamableField(const F& field,
                    NullEncoder enc = {})
    : _field(field)
    , _encoder(std::move(enc))
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamableField& field) {
        FormattedAsciiOutputStream formatted_stream{s, field._opts};
        field._field.stream(formatted_stream);
        return s;
    }

 private:
    const F& _field;
    Encoder _encoder;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_