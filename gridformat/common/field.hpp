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

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/streams.hpp>

namespace GridFormat {

class FieldVisitor {
 public:
    void take_field_values(const DynamicPrecision& prec,
                           const std::byte* data,
                           const std::size_t size) {
        if (size%prec.number_of_bytes() != 0)
            throw InvalidState("Number of bytes is not an integer multiple of given precision");
        _take_field_values(prec, data, size);
    }

 private:
    virtual void _take_field_values(const DynamicPrecision&, const std::byte*, const std::size_t) {
        throw NotImplemented("Visitor does not implement take_field_values()");
    }
};

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class Field {
 public:
    using Serialization = GridFormat::Serialization;

    virtual ~Field() = default;
    Field(MDLayout layout, DynamicPrecision prec)
    : _layout(std::move(layout))
    , _prec(std::move(prec))
    {}

    const MDLayout& layout() const { return _layout; }
    DynamicPrecision precision() const { return _prec; }
    void visit(FieldVisitor& v) const { _visit(v); }

 private:
    virtual void _visit(FieldVisitor& v) const = 0;

    MDLayout _layout;
    DynamicPrecision _prec;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_