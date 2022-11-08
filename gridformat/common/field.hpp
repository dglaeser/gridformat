// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_FIELD_HPP_
#define GRIDFORMAT_COMMON_FIELD_HPP_

#include <concepts>
#include <ostream>
#include <utility>
#include <cstddef>

#include <gridformat/common/precision.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class Field {
 public:
    using Serialization = std::vector<std::byte>;

    virtual ~Field() = default;
    explicit Field(int ncomps, DynamicPrecision prec)
    : _ncomps(ncomps)
    , _prec(prec)
    {}

    DynamicPrecision precision() const { return _prec; }
    int number_of_components() const { return _ncomps; }

    void stream(std::ostream& s) const { _stream(s); }
    Serialization serialized() const { return _serialized(); }

 private:
    int _ncomps;
    DynamicPrecision _prec;

    virtual void _stream(std::ostream&) const = 0;
    virtual Serialization _serialized() const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_