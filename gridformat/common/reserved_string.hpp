// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::ReservedString
 */
#ifndef GRIDFORMAT_COMMON_RESERVED_STRING_HPP_
#define GRIDFORMAT_COMMON_RESERVED_STRING_HPP_

#include <cmath>
#include <string_view>
#include <algorithm>
#include <ostream>

#if __has_include(<format>)
#include <format>
#endif

#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief String with a fixed maximum size of characters it can hold.
 *        We need to construct constexpr variables of strings in some places,
 *        which is not possible with std::string. This implementation currently
 *        only exposes the minimum interface that we need.
 * \note Does not support dynamic allocation to more characters than max_size.
 */
template<std::size_t max_size = 30>
class ReservedString {
 public:
    ReservedString() = default;

    template<std::size_t N>
    constexpr ReservedString(const char (&input)[N]) {
        static_assert(N - 1 <= max_size, "Given string literal is too long");
        if (input[N-1] != '\0')
            throw ValueError("Expected null terminator character at the end of string literal");
        _copy_from(input, N-1);
    }

    constexpr ReservedString(std::string_view s) {
        _check_size(s.size());
        _copy_from(s.begin(), s.size());
    }

    constexpr ReservedString(const std::string& s)
    : ReservedString(std::string_view{s})
    {}

    friend constexpr bool operator==(const ReservedString& a, const ReservedString& b) {
        if (a._size != b._size)
            return false;
        return std::equal(a._text, a._text + a._size, b._text);
    }

    friend std::ostream& operator<<(std::ostream& s, const ReservedString& str) {
        std::for_each_n(str._text, str._size, [&] (const auto& character) {
            s.put(character);
        });
        return s;
    }

    constexpr operator std::string_view() const {
        return std::string_view{_text, _text + _size};
    }

    constexpr std::size_t size() const { return _size; }
    constexpr auto begin() const { return _text; }
    constexpr auto end() const { return _text + _size; }

 private:
    template<typename IT>
    constexpr void _copy_from(IT&& iterator, std::size_t n) {
        std::copy_n(std::forward<IT>(iterator), std::min(n, max_size), _text);
        _size = n;
    }

    void _check_size(std::size_t n) {
        if (n >= max_size)
            throw SizeError(
                "Given character sequence exceeds maximum of " + std::to_string(max_size) + " characters "
                + "(has" + std::to_string(n) + " characters). Input will be cropped."
            );
    }

    char _text[max_size];
    unsigned int _size{0};
};

template<std::size_t N>
ReservedString(const char (&input)[N]) -> ReservedString<N-1>;

}  // namespace GridFormat

#if __cpp_lib_format
// specialize std::formatter for GridFormat::ReservedString
template <std::size_t n>
struct std::formatter<GridFormat::ReservedString<n>> : std::formatter<std::string_view> {
    auto format(const GridFormat::ReservedString<n>& s, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(static_cast<std::string_view>(s), ctx);
    }
};
#endif

#endif  // GRIDFORMAT_COMMON_RESERVED_STRING_HPP_
