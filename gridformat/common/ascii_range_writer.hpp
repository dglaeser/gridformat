// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ASCII_RANGE_WRITER_HPP_
#define GRIDFORMAT_COMMON_ASCII_RANGE_WRITER_HPP_

#include <string>
#include <utility>
#include <ranges>
#include <iostream>

#include <gridformat/common/logging.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me (mention max_chars_per_line neglects indentation)
 */
struct RangeFormattingOptions {
    std::string delimiter = " ";
    std::string line_prefix = "";
    std::size_t num_entries_per_line = 10;
};

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class AsciiRangeWriter {
 public:
    explicit AsciiRangeWriter(RangeFormattingOptions opts = {})
    : _opts(std::move(opts)) {
        if (_opts.num_entries_per_line == 0) {
            Logging::as_warning("(AsciiRangeWriter) 0 entries per line specified, setting to 1");
            _opts.num_entries_per_line = 1;
        }
    }

    template<std::ranges::range R>
    void write(R&& input_range, std::ostream& stream) const {
        std::size_t entries_per_line = 0;
        for (const auto& value : input_range) {
            if (entries_per_line == _opts.num_entries_per_line) {
                stream << "\n";
                entries_per_line = 0;
            }
            stream << (entries_per_line == 0 ? _opts.line_prefix : _opts.delimiter);
            stream << value;
            entries_per_line++;
        }
    }

 private:
    RangeFormattingOptions _opts;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ASCII_RANGE_WRITER_HPP_