// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper for writing into files with error checking.
 */
#ifndef GRIDFORMAT_COMMON_OUTPUT_FILE_HPP_
#define GRIDFORMAT_COMMON_OUTPUT_FILE_HPP_

#include <string>
#include <ostream>
#include <fstream>
#include <concepts>

#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Open the file with the given name, pass it to the given callback and close it.
 *        Throws an IOError if the file cannot be opened or if writing to it fails.
 */
template<std::invocable<std::ostream&> C>
void write_to(const std::string& filename, C&& callback) {
    std::ofstream file{filename, std::ios::out};
    if (!file.is_open())
        throw IOError("Could not open file '" + filename + "' for writing");
    callback(file);
    file.close();
    if (file.fail())
        throw IOError("Error while writing file '" + filename + "'");
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_OUTPUT_FILE_HPP_
