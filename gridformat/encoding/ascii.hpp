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
#include <concepts>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <string>
#include <span>
#include <sstream>

#if __has_include(<format>)
#include <format>
#endif

#include <gridformat/common/output_stream.hpp>
#include <gridformat/common/reserved_string.hpp>

#ifndef DOXYGEN
namespace GridFormat::Encoding::Detail {

    template<typename T>
    struct AsciiPrintType : std::type_identity<T> {};
    template<std::signed_integral T>
    struct AsciiPrintType<T> : std::type_identity<std::intmax_t> {};
    template<std::unsigned_integral T>
    struct AsciiPrintType<T> : std::type_identity<std::uintmax_t> {};

}  // namespace GridFormat::Encoding::Detail
#endif  // DOXYGEN

namespace GridFormat {

//! \addtogroup Encoding
//! \{

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
    class Buffer {
     public:
        Buffer(std::streamsize precision)
        : _precision{precision} {
            _init_stream_buf();
        }

        template<typename V, typename D>
        void push(V&& value, D&& delimiter) {
#if __cpp_lib_format
            if constexpr (std::floating_point<std::remove_cvref_t<V>>)
                std::format_to(std::back_inserter(_string_buf),
                    "{:.{}g}{}", std::forward<V>(value), _precision, std::forward<D>(delimiter)
                );
            else
                std::format_to(std::back_inserter(_string_buf),
                    "{}{}", std::forward<V>(value), std::forward<D>(delimiter)
                );
#else
            _stream_buf << std::forward<V>(value) << std::forward<D>(delimiter);
#endif
        }

        void prepare_readout() {
#if !__cpp_lib_format
            // move internal string buffer out of the stream
            // (see https://stackoverflow.com/a/66662433)
            _string_buf = std::move(_stream_buf).str();
            _init_stream_buf(); // reinitialize
#endif
        }

        auto data() const {
            return std::span{_string_buf.data(), _string_buf.size()};
        }

        void clear() {
            _string_buf.clear();
        }

     private:
        void _init_stream_buf() {
            _stream_buf = {};
            _stream_buf.precision(_precision);
        }

        std::streamsize _precision;
        std::string _string_buf;
        std::ostringstream _stream_buf;
    };

 public:
    AsciiOutputStream(OStream& s, AsciiFormatOptions opts = {})
    : OutputStreamWrapperBase<OStream>(s)
    , _opts{std::move(opts)}
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        std::size_t count_entries = 0;
        std::size_t count_buffer_lines = 0;

        using PrintType = typename Encoding::Detail::AsciiPrintType<T>::type;
        Buffer buffer(std::numeric_limits<PrintType>::digits10);
        while (count_entries < data.size()) {
            buffer.push(count_entries > 0 ? "\n" : "", _opts.line_prefix);

            using std::min;
            const auto num_entries = min(_opts.entries_per_line, data.size() - count_entries);
            for (const auto& value : data.subspan(count_entries, num_entries))
                buffer.push(static_cast<PrintType>(value), _opts.delimiter);

            // update counters
            count_entries += num_entries;
            ++count_buffer_lines;

            // flush and reset buffer
            if (count_buffer_lines >= _opts.num_cached_lines) {
                buffer.prepare_readout();
                this->_write_raw(buffer.data());
                buffer.clear();
                count_buffer_lines = 0;
            }
        }

        // flush remaining buffer content
        buffer.prepare_readout();
        this->_write_raw(buffer.data());
    }

    AsciiFormatOptions _opts;
};

//! \} group Encoding

}  // namespace GridFormat

namespace GridFormat::Encoding {

//! \addtogroup Encoding
//! \{

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

//! Instance of the ascii encoder
inline constexpr Ascii ascii;

//! \} group Encoding

}  // namespace GridFormat::Encoding

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
