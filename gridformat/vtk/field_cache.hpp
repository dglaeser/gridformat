// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTK::FieldCache
 */
#ifndef GRIDFORMAT_VTK_FIELD_CACHE_HPP_
#define GRIDFORMAT_VTK_FIELD_CACHE_HPP_

#include <list>
#include <memory>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/transformed_fields.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief Storage class for fields in VTK.
 *        VTK requires that vector/tensor fields are extended to
 *        3d. This class allows insertion of fields, which are then
 *        automatically transformed into 3d fields. Writers can use
 *        this class temporarily in the `_write` function.
 */
class FieldCache {
 public:
    template<typename F>
    requires(std::is_lvalue_reference_v<F> and std::derived_from<std::decay_t<F>, Field>)
    const Field& insert(F&& field) {
        // vector/tensor fields must be made 3d
        if (field.layout().dimension() > 1)
            return *_fields.emplace_back(_make_unique(
                TransformedField{std::forward<F>(field), FieldTransformation::extend_all_to(3)}
            ));
        // scalar fields are just wrapped in an identity transformation
        else
            return *_fields.emplace_back(_make_unique(
                TransformedField{std::forward<F>(field), FieldTransformation::identity}
            ));
    }

 private:
    std::list<std::unique_ptr<const Field>> _fields;

    template<typename T> requires(!std::is_lvalue_reference_v<T>)
    auto _make_unique(T&& t) const {
        return std::make_unique<std::decay_t<T>>(std::move(t));
    }
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_FIELD_CACHE_HPP_
