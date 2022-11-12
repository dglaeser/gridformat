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
#include <cassert>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class Field {
 public:
    virtual ~Field() = default;
    Field(MDLayout layout, DynamicPrecision prec)
    : _layout(std::move(layout))
    , _prec(std::move(prec))
    {}

    std::size_t dimension() const { return _layout.dimension(); }
    std::size_t number_of_entries() const { return _layout.number_of_entries(); }
    std::size_t number_of_entries(std::size_t codim) const { return _layout.number_of_entries(codim); }
    std::size_t extent(std::size_t codim) const { return _layout.extent(codim); }
    std::size_t size_in_bytes() const { return number_of_entries()*precision().number_of_bytes(); }
    const DynamicPrecision& precision() const { return _prec; }

    Serialization serialized() const {
        auto serialization = _serialized();
        assert(serialization.size() == size_in_bytes());
        return serialization;
    }

 private:
    virtual Serialization _serialized() const = 0;

    MDLayout _layout;
    DynamicPrecision _prec;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_