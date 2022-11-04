// Copyright [2022] <Dennis Gläser>
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

/*!
 * \file
 * \ingroup Common
 * \brief Exception handling facilities.
 */
#ifndef GRIDFORMAT_COMMON_EXCEPTIONS_HPP_
#define GRIDFORMAT_COMMON_EXCEPTIONS_HPP_

#include <source_location>
#include <concepts>
#include <exception>
#include <utility>
#include <string_view>
#include <string>

namespace GridFormat {

//! \addtogroup Common
//! \{

//! Base class for exceptions in GridFormat
class Exception : public std::exception {
 public:
    explicit Exception(std::string_view what,
                       const std::source_location loc = std::source_location::current()) {
        _what = what;
        _what += "\n";
        _what += "\tFunction: " + std::string(loc.function_name()) + "\n";
        _what += "\tFile:     " + std::string(loc.file_name()) + "\n";
        _what += "\tLine:     " + std::to_string(loc.line()) + "\n";
    }

    const char* what() const noexcept override {
        return _what.data();
    }

 private:
    std::string _what;
};

class NotImplemented : public Exception {
 public:
    using Exception::Exception;
};

class InvalidState : public Exception {
 public:
    using Exception::Exception;
};

//! \} group Common

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_EXCEPTIONS_HPP_