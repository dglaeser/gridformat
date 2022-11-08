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
#include <gridformat/common/serialization.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
template<typename S = GridFormat::Serialization>;
class Field {
 public:
    using Serialization = S;

    virtual ~Field() = default;
    explicit Field(int ncomps, PrecisionTraits prec)
    : _ncomps(ncomps)
    , _prec(prec)
    {}

    PrecisionTraits precision() const { return _prec; }
    int number_of_components() const { return _ncomps; }
    Serialization serialized() const { return _serialized(); }

 private:
    int _ncomps;
    PrecisionTraits _prec;
    virtual Serialization _serialized() const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_