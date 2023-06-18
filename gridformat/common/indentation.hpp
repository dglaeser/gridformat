// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Indentation.
 */
#ifndef GRIDFORMAT_COMMON_INDENTATION_HPP_
#define GRIDFORMAT_COMMON_INDENTATION_HPP_

#include <string>
#include <ostream>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Helper class for formatting output with indentations.
 */
class Indentation {
 public:
    struct Options {
        unsigned int width = 4;
        unsigned int level = 0;
    };

    Indentation() : Indentation(Options{}) {}
    explicit Indentation(Options opts)
    : _width{std::string(opts.width, ' ')}
    , _indent{std::string(opts.width*opts.level, ' ')}
    {}

    const std::string& get() const { return _indent; }

    Indentation& operator++() { push_(); return *this; }
    Indentation& operator--() { pop_(); return *this; }

    Indentation operator++(int) { auto cpy = *this; ++(*this); return cpy; }
    Indentation operator--(int) { auto cpy = *this; --(*this); return cpy; }

    friend std::ostream& operator<<(std::ostream& s, const Indentation& i) {
        s << i._indent;
        return s;
    }

 private:
    void push_() { _indent += _width; }
    void pop_() { _indent.erase(std::max(_indent.length() - _width.length(), std::size_t{0})); }

    std::string _width;
    std::string _indent;
};

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_INDENTATION_HPP_
