// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
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
        std::vector<std::string_view> result;
        const std::string_view chars{_chars};
        std::size_t begin = 0;
        for (std::size_t pos = chars.find('\0'); pos != std::string_view::npos; pos = chars.find('\0', begin)) {
            result.push_back(chars.substr(begin, pos - begin));
            begin = pos + 1;
        }
        if (begin < chars.size() || result.empty())
            result.push_back(chars.substr(begin));
        return result;
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
