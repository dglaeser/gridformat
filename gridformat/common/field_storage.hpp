// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_
#define GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_

#include <memory>
#include <string>
#include <utility>
#include <unordered_map>

#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<typename _F = Field>
class FieldStorage {
 public:
    using Field = _F;

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set(const std::string& name, F&& field) {
        _fields.insert_or_assign(name, std::make_unique<F>(std::move(field)));
    }

    const Field& get(const std::string& name) const {
        return *(_fields.at(name));
    }

 private:
    std::unordered_map<std::string, std::unique_ptr<Field>> _fields;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_