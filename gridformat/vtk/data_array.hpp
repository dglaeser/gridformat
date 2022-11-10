// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_DATA_ARRAY_HPP_
#define GRIDFORMAT_VTK_DATA_ARRAY_HPP_

#include <memory>
#include <utility>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/range_formatter.hpp>
#include <gridformat/common/streams.hpp>

namespace GridFormat::VTK {

template<typename F,
         typename Encoder,
         typename Compressor>
class DataArray;

template<std::derived_from<Field> F,
         typename Encoder,
         typename Compressor>
class StreamableField<F> {
 public:
    StreamableField(const F& field, RangeFormatOptions opts = {})
    : _field(field)
    , _opts(std::move(opts))
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamableField& field) {
        FormattedAsciiOutputStream formatted_stream{s, field._opts};
        field._field.stream(formatted_stream);
        return s;
    }

 private:
    const F& _field;
    RangeFormatOptions _opts;
};

template<std::derived_from<Field> F>
class StreamableField<std::unique_ptr<F>> {
 public:
    StreamableField(F&& field, RangeFormatOptions opts = {})
    : _field_ptr(std::make_unique<F>(std::move(field)))
    , _opts(std::move(opts))
    {}

    friend std::ostream& operator<<(std::ostream& s, const StreamableField& field) {
        FormattedAsciiOutputStream formatted_stream{s, field._opts};
        field._field_ptr->stream(formatted_stream);
        return s;
    }

 private:
    std::unique_ptr<F> _field_ptr;
    RangeFormatOptions _opts;
};

template<typename F> requires(
    std::derived_from<std::decay_t<F>, Field> and
    std::is_lvalue_reference_v<F>)
auto make_streamable(F&& field, RangeFormatOptions opts = {}) {
    return StreamableField<std::decay_t<F>>{field, std::move(opts)};
}

template<typename F> requires(
    std::derived_from<std::decay_t<F>, Field> and
    !std::is_lvalue_reference_v<F>)
auto make_streamable(F&& field, RangeFormatOptions opts = {}) {
    return StreamableField<std::unique_ptr<std::decay_t<F>>>{std::move(field), std::move(opts)};
}

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_DATA_ARRAY_HPP_