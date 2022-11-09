// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_FIELD_HPP_
#define GRIDFORMAT_COMMON_FIELD_HPP_

#include <utility>
#include <cstddef>
#include <ostream>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/streams.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class Field {
 public:
    using Serialization = GridFormat::Serialization;

    virtual ~Field() = default;
    Field(MDLayout layout, PrecisionTraits prec)
    : _layout(std::move(layout))
    , _prec(prec)
    {}

    const MDLayout& layout() const { return _layout; }
    PrecisionTraits precision() const { return _prec; }
    Serialization serialized() const { return _serialized(); }
    void stream(FormattedAsciiOutputStream& s) const { _stream(s); }

 private:
    MDLayout _layout;
    PrecisionTraits _prec;
    virtual Serialization _serialized() const = 0;
    virtual void _stream(FormattedAsciiOutputStream&) const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_