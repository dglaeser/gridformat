// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_STREAMS_HPP_
#define GRIDFORMAT_COMMON_STREAMS_HPP_

#include <utility>
#include <ostream>
#include <cstddef>
#include <string>

#include <gridformat/common/logging.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/range_formatter.hpp>

namespace GridFormat {

struct RangeFormatOptions {
    std::string delimiter = " ";
    std::string line_prefix = "";
    std::size_t num_entries_per_line = 10;
};

template<typename Stream = std::ostream>
class FormattedAsciiStream {
    template<typename T>
    struct Print : std::type_identity<T> {};
    template<std::signed_integral T>
    struct Print<T> : std::type_identity<std::intmax_t> {};
    template<std::unsigned_integral T>
    struct Print<T> : std::type_identity<std::uintmax_t> {};

 public:
    explicit FormattedAsciiStream(Stream& s, RangeFormatOptions opts = {})
    : _stream(s)
    , _opts(std::move(opts)) {
        if (_opts.num_entries_per_line == 0) {
            Logging::as_warning("(RangeFormatter) 0 entries per line specified, setting to 1");
            _opts.num_entries_per_line = 1;
        }
    }

    template<Concepts::Streamable<Stream> T> requires(Concepts::Streamable<std::string, Stream>)
    friend FormattedAsciiStream& operator<<(FormattedAsciiStream& s, const T& value) {
        if (s._entries_on_current_line == s._opts.num_entries_per_line) {
            s._stream << "\n";
            s._entries_on_current_line = 0;
        }
        s._stream << (s._entries_on_current_line == 0 ? s._opts.line_prefix : s._opts.delimiter);
        s._stream << static_cast<typename Print<T>::type>(value);
        s._entries_on_current_line++;
        return s;
    }

 private:
    Stream& _stream;
    RangeFormatOptions _opts;
    std::size_t _entries_on_current_line = 0;
};

using FormattedAsciiOutputStream = FormattedAsciiStream<std::ostream>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAMS_HPP_