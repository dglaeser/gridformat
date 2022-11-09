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

#include <gridformat/common/range_formatter.hpp>
#include <gridformat/common/streams.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class StreamableField {
 public:
    template<typename F> requires(
        std::derived_from<std::decay_t<F>, Field> or
        std::is_lvalue_reference_v<F>)
    StreamableField(F&& field, RangeFormatOptions opts = {})
    : _field(field)
    , _opts(std::move(opts))
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamableField& field) {
        FormattedAsciiOutputStream formatted_stream{s, field._opts};
        field._field.stream(formatted_stream);
        return s;
    }

 private:
    const Field& _field;
    RangeFormatOptions _opts;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAMABLE_FIELD_HPP_