// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::EmptyField
 */
#ifndef GRIDFORMAT_COMMON_EMPTY_FIELD_HPP_
#define GRIDFORMAT_COMMON_EMPTY_FIELD_HPP_

#include <gridformat/common/field.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat {

class EmptyField : public Field {
 public:
    explicit EmptyField(DynamicPrecision p) : _prec{p} {}

 private:
    MDLayout _layout() const { return MDLayout{}; }
    DynamicPrecision _precision() const { return _prec; }
    Serialization _serialized() const { return Serialization{}; }
    DynamicPrecision _prec;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_EMPTY_FIELD_HPP_
