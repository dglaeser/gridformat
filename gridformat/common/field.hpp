// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Field
 */
#ifndef GRIDFORMAT_COMMON_FIELD_HPP_
#define GRIDFORMAT_COMMON_FIELD_HPP_

#include <memory>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Interface for fields of values.
 */
class Field {
 public:
    virtual ~Field() = default;

    MDLayout layout() const { return _layout(); }
    DynamicPrecision precision() const { return _precision(); }
    Serialization serialized() const { return _serialized(); }
    std::size_t size_in_bytes() const { return layout().number_of_entries()*precision().size_in_bytes(); }

 private:
    DynamicPrecision _prec;

    virtual MDLayout _layout() const = 0;
    virtual DynamicPrecision _precision() const = 0;
    virtual Serialization _serialized() const = 0;
};

using FieldPtr = std::shared_ptr<const Field>;

template<typename F> requires(
    std::derived_from<std::decay_t<F>, Field> and
    !std::is_lvalue_reference_v<F>)
FieldPtr make_shared(F&& f) {
    return std::make_shared<F>(std::forward<F>(f));
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_HPP_
