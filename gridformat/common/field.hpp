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

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields.
 */
class Field {
 public:
    using Serialization = std::vector<std::byte>;

    virtual ~Field() = default;

    void stream(std::ostream& s) const { _stream(s); }
    Serialization serialized() const { return _serialized(); }

 private:
    virtual void _stream(std::ostream&) const = 0;
    virtual Serialization _serialized() const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_