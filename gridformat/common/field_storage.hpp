// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <unordered_map>

#include <gridformat/common/field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Class to store field instances by name.
 */
template<typename _F = Field>
class FieldStorage {
 public:
    using Field = _F;

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set(const std::string& name, F&& field) {
        _fields.insert_or_assign(name, make_shared(std::move(field)));
    }

    void set(const std::string& name, FieldPtr field_ptr) {
        _fields.insert_or_assign(name, field_ptr);
    }

    const Field& get(const std::string& name) const {
        return *(_fields.at(name));
    }

    std::shared_ptr<const Field> get_shared(const std::string& name) const {
        return _fields.at(name);
    }

    std::ranges::range auto field_names() const {
        return std::views::keys(_fields);
    }

    void clear() {
        _fields.clear();
    }

 private:
    std::unordered_map<std::string, FieldPtr> _fields;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_STORAGE_HPP_
