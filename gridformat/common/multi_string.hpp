// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::MultiString
 */
#ifndef GRIDFORMAT_COMMON_MULTI_STRING_HPP_
#define GRIDFORMAT_COMMON_MULTI_STRING_HPP_

#include <string>
#include <string_view>
#include <ranges>
#include <vector>
#include <cstddef>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief A sequence of strings, stored contiguously with each string terminated by '\0'.
 * \details This is the form in which fields hold multiple strings, e.g. string meta data. It is a
 *          range of characters, so it can be passed as meta data and fields can be exported into it.
 *          A trailing string without terminator counts as a string, and an empty sequence of characters
 *          represents a single empty string, just like an empty std::string.
 */
class MultiString {
 public:
    using value_type = char;

    MultiString() = default;

    explicit MultiString(const std::vector<std::string>& strings) {
        for (const auto& s : strings) {
            _chars.append(s);
            _chars.push_back('\0');
        }
    }

    //! Return views on the individual strings (not available on temporaries, the views would dangle)
    std::vector<std::string_view> slices() const && = delete;
    std::vector<std::string_view> slices() const & {
        const bool has_trailing_nul = !_chars.empty() && _chars.back() == '\0';
        auto data = has_trailing_nul ? std::string_view{_chars.data(), _chars.size() - 1} : std::string_view{_chars};
        auto slices = std::views::split(data, '\0') | std::views::transform([] (auto&& s) {
            return std::string_view(std::ranges::begin(s), std::ranges::end(s));
        });
        if (std::ranges::empty(slices))
            return {{""}};
        return std::vector<std::string_view>{std::ranges::begin(slices), std::ranges::end(slices)};
    }

    std::size_t size() const { return _chars.size(); }
    const char* data() const { return _chars.data(); }
    void resize(std::size_t n, char c = '\0') { _chars.resize(n, c); }

    auto begin() { return _chars.begin(); }
    auto begin() const { return _chars.begin(); }
    auto end() { return _chars.end(); }
    auto end() const { return _chars.end(); }

    friend bool operator==(const MultiString&, const MultiString&) = default;

 private:
    std::string _chars;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MULTI_STRING_HPP_
