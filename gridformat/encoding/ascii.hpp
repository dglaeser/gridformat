// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream using ascii
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
#define GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_

#include <cmath>
#include <limits>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <string>
#include <span>

#include <format>

#include <gridformat/common/output_stream.hpp>
#include <gridformat/common/reserved_string.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

#ifndef DOXYGEN
namespace Encoding::Detail {

    template<typename T>
    struct AsciiPrintType : std::type_identity<T> {};
    template<std::signed_integral T>
    struct AsciiPrintType<T> : std::type_identity<std::intmax_t> {};
    template<std::unsigned_integral T>
    struct AsciiPrintType<T> : std::type_identity<std::uintmax_t> {};

}  // namespace Encoding::Detail
#endif  // DOXYGEN

//! Options for fomatted output of ranges with ascii encoding
struct AsciiFormatOptions {
    ReservedString<30> delimiter{""};
    ReservedString<30> line_prefix{""};
    std::size_t entries_per_line = std::numeric_limits<std::size_t>::max();
    std::size_t num_cached_lines = 100; //!< Number of line cached between flushing the buffer

    friend bool operator==(const AsciiFormatOptions& a, const AsciiFormatOptions& b) {
        return a.delimiter == b.delimiter
            && a.line_prefix == b.line_prefix
            && a.entries_per_line == b.entries_per_line
            && a.num_cached_lines == b.num_cached_lines;
    }
};

//! Wrapper around a given stream to write formatted ascii output
template<typename OStream>
class AsciiOutputStream : public OutputStreamWrapperBase<OStream> {
 public:
    AsciiOutputStream(OStream& s, AsciiFormatOptions opts = {})
    : OutputStreamWrapperBase<OStream>(s)
    , _opts{std::move(opts)}
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        std::size_t count_entries = 0;
        std::size_t count_buffer_lines = 0;
        std::string buffer;

        while (count_entries < data.size()) {
            // format one line to buffer
            std::format_to(std::back_inserter(buffer),
                "{}{}", count_entries > 0 ? "\n" : "", _opts.line_prefix
            );

            using std::min;
            const auto num_entries = min(_opts.entries_per_line, data.size() - count_entries);
            const auto sub_range = data.subspan(count_entries, num_entries);
            using PrintType = typename Encoding::Detail::AsciiPrintType<T>::type;
            for (const auto& value : sub_range)
                std::format_to(std::back_inserter(buffer),
                    "{}{}", static_cast<PrintType>(value), _opts.delimiter
                );

            // update counters
            count_entries += num_entries;
            ++count_buffer_lines;

            // flush and reset buffer
            if (count_buffer_lines >= _opts.num_cached_lines) {
                this->_write_raw(std::span{buffer.c_str(), buffer.size()});
                buffer.clear();
                count_buffer_lines = 0;
            }
        }

        // flush remaining buffer content
        this->_write_raw(std::span{buffer.c_str(), buffer.size()});
    }

    AsciiFormatOptions _opts;
};

namespace Encoding {

//! Ascii encoder
struct Ascii {
    constexpr Ascii() = default;
    constexpr explicit Ascii(AsciiFormatOptions opts)
    : _opts{std::move(opts)}
    {}

    //! Create an ascii stream with the defined options
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return AsciiOutputStream{s, options()};
    }

    //! Return a new instance with different options
    static constexpr auto with(AsciiFormatOptions opts) {
        return Ascii{std::move(opts)};
    }

    //! Return the current options
    constexpr AsciiFormatOptions options() const {
        return _opts.value_or(AsciiFormatOptions{});
    }

 private:
    // we use optional here in order to be able to define
    // an inline constexpr instance of this class below
    std::optional<AsciiFormatOptions> _opts = {};
};

inline constexpr Ascii ascii;  //!< Instance of the ascii encoder

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
