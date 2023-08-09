// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::LValueReferenceOf
 */
#ifndef GRIDFORMAT_COMMON_LVALUE_REFERENCE_HPP_
#define GRIDFORMAT_COMMON_LVALUE_REFERENCE_HPP_

#include <type_traits>
#include <concepts>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Helper class that can be used in interfaces that require lvalue references.
 * \details Since temporaries of T also bind to const T&, interfaces (e.g. constructors)
 *          that take a reference and store it can lead to dangling references when called
 *          with temporaries. In those places, using this helper type yields compile-time
 *          errors in such cases.
 */
template<typename T>
class LValueReferenceOf {
 public:
    template<typename _T>
        requires(std::same_as<std::remove_const_t<T>, std::remove_cvref_t<_T>>)
    LValueReferenceOf(_T&& ref) : _ref{ref} {
        static_assert(
            std::is_lvalue_reference_v<_T>,
            "Cannot bind a temporary to an lvalue reference."
        );
    }

    T& get() { return _ref; }
    std::add_const_t<T>& get() const { return _ref; }

 private:
    T& _ref;
};

template<typename T>
LValueReferenceOf(T&&) -> LValueReferenceOf<std::remove_reference_t<T>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_LVALUE_REFERENCE_HPP_
