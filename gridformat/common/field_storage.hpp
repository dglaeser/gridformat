// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::FieldStorage
 */
#ifndef GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_
#define GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_

#include <string>
#include <ranges>
#include <utility>
#include <map>

#include <gridformat/common/field.hpp>
#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Class to store field instances by name.
 */
class FieldStorage {
 public:
    using Field = GridFormat::Field;
    using FieldPtr = GridFormat::FieldPtr;

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set(const std::string& name, F&& field) {
        _fields.insert_or_assign(name, make_field_ptr(std::move(field)));
    }

    void set(const std::string& name, FieldPtr field_ptr) {
        _fields.insert_or_assign(name, field_ptr);
    }

    const Field& get(const std::string& name) const {
        return *(get_ptr(name));
    }

    FieldPtr get_ptr(const std::string& name) const {
        if (!_fields.contains(name))
            throw ValueError("No field with name " + name);
        return _fields.at(name);
    }

    std::ranges::range auto field_names() const {
        return std::views::keys(_fields);
    }

    FieldPtr pop(const std::string& name) {
        auto field = get_ptr(name);
        _fields.erase(name);
        return field;
    }

    void clear() {
        _fields.clear();
    }

 private:
    std::map<std::string, FieldPtr> _fields;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_
