// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief GridFormat::OptionalReference
 */
#ifndef GRIDFORMAT_COMMON_OPTIONAL_REFERENCE_HPP_
#define GRIDFORMAT_COMMON_OPTIONAL_REFERENCE_HPP_

#include <type_traits>
#include <functional>
#include <optional>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Contains an optional reference to T.
 *        Can be used e.g. in search algorithms that, if successful,
 *        return a reference to T and none if the search failed.
 */
template<typename T>
class OptionalReference {
 public:
    OptionalReference() = default;
    OptionalReference(T& ref) : _ref{ref} {}

    void release() { _ref.reset(); }

    T& unwrap() requires(!std::is_const_v<T>) { return _ref.value(); }
    const T& unwrap() const { return _ref.value(); }

    bool has_value() const { return _ref.has_value(); }
    operator bool() const { return _ref.has_value(); }

 private:
    std::optional<std::reference_wrapper<T>> _ref;
};

template<typename T>
OptionalReference(T&) -> OptionalReference<T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_OPTIONAL_REFERENCE_HPP_
