// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_RANGE_FORMATTER_HPP_
#define GRIDFORMAT_COMMON_RANGE_FORMATTER_HPP_

#include <utility>
#include <ostream>
#include <ranges>
#include <cstddef>
#include <string>

#include <gridformat/common/logging.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/range_formatter.hpp>

namespace GridFormat {

struct RangeFormatterOptions {
    std::string delimiter = " ";
    std::string line_prefix = "";
    std::size_t num_entries_per_line = 10;
};

class RangeFormatter {
 public:
    explicit RangeFormatter(RangeFormatterOptions opts = {})
    : _opts(std::move(opts)) {
        if (_opts.num_entries_per_line == 0) {
            Logging::as_warning("(RangeFormatter) 0 entries per line specified, setting to 1");
            _opts.num_entries_per_line = 1;
        }
    }

    template<std::ranges::range R> requires(
        Concepts::Streamable<std::ranges::range_value_t<R>>)
    void write(std::ostream& stream, R&& input_range) const {
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
    RangeFormatterOptions _opts;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_RANGE_FORMATTER_HPP_