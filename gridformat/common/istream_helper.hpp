// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper for parsing data from input streams.
 */
#ifndef GRIDFORMAT_COMMON_ISTREAM_HELPER_HPP_
#define GRIDFORMAT_COMMON_ISTREAM_HELPER_HPP_

#include <cmath>
#include <string>
#include <istream>
#include <optional>
#include <utility>
#include <concepts>
#include <iterator>
#include <limits>

#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Helper for parsing data from input streams.
 */
class InputStreamHelper {
 public:
    static constexpr std::size_t default_chunk_size = 5000;

    explicit InputStreamHelper(std::istream& s, std::string whitespace_chars = " \n\t")
    : _stream{s}
    , _whitespace_chars{std::move(whitespace_chars)}
    {}

    //! Read a chunk of characters from the stream into the given buffer
    void read_chunk_to(std::string& buffer, const std::size_t chunk_size = default_chunk_size) {
        if (is_end_of_file())
            throw IOError("End of file already reached");
        buffer.resize(chunk_size);
        _stream.read(buffer.data(), chunk_size);
        _stream.clear();
        buffer.resize(_stream.gcount());
    }

    //! Read a chunk of characters from the stream
    std::string read_chunk(const std::size_t chunk_size = default_chunk_size) {
        std::string tmp(chunk_size, ' ');
        read_chunk_to(tmp, chunk_size);
        return tmp;
    }

    //! Move the position forward until any of the given characters is found or EOF is reached
    bool shift_until_any_of(const std::string& chars, std::optional<std::size_t> max_chars = {}) {
        std::string tmp_buffer;
        std::size_t char_count = 0;
        const auto max_num_chars = max_chars.value_or(std::numeric_limits<std::size_t>::max());

        while (char_count < max_num_chars) {
            read_chunk_to(tmp_buffer, std::min(default_chunk_size, max_num_chars - char_count));
            const auto str_pos = tmp_buffer.find_first_of(chars);
            if (str_pos != std::string::npos) {
                const auto delta_pos = tmp_buffer.size() - str_pos;
                shift_by(-delta_pos);
                return true;
            }

            if (is_end_of_file())
                return false;

            char_count += tmp_buffer.size();
        }

        return false;
    }

    //! Read characters from the stream until any of the given characters is found or EOF is reached
    std::string read_until_any_of(const std::string& chars, std::optional<std::size_t> max_chars = {}) {
        const auto p0 = position();
        shift_until_any_of(chars, max_chars);
        const auto p1 = position();
        seek_position(p0);
        return read_chunk(p1 - p0);
    }

    //! Move the position forward until a character that is none of the given ones is found or EOF is reached
    bool shift_until_not_any_of(const std::string& chars) {
        std::string tmp_buffer;
        while (true) {
            tmp_buffer = read_chunk();
            const auto pos = tmp_buffer.find_first_not_of(chars);
            if (pos != std::string::npos) {
                const auto delta_pos = tmp_buffer.size() - pos;
                shift_by(-delta_pos);
                return true;
            }

            if (is_end_of_file())
                return false;
        }
    }

    //! Read from the stream until a character not matching any of the given characters is found or EOF is reached
    std::string read_until_not_any_of(const std::string& chars) {
        const auto p0 = position();
        shift_until_not_any_of(chars);
        const auto p1 = position();
        seek_position(p0);
        return read_chunk(p1 - p0);
    }

    //! Move the position until the given string is found or EOF is reached
    bool shift_until_substr(const std::string& substr) {
        const std::streamsize delta = -substr.size();
        const auto chunk_size = std::max(substr.size()*10, default_chunk_size);
        std::string chunk;
        while (true) {
            const auto cur_pos = position();
            read_chunk_to(chunk, chunk_size);
            const auto substr_pos = chunk.find(substr);
            if (substr_pos != std::string::npos) {
                seek_position(cur_pos + substr_pos);
                return true;
            }

            if (is_end_of_file())
                return false;

            // shift back a bit in case the substr lies at chunk boundaries
            shift_by(delta);
        }
    }

    //! Skip characters considered whitespace
    void shift_whitespace() {
        shift_until_not_any_of(_whitespace_chars);
    }

    //! Skip characters until a whitespace is found
    void shift_until_whitespace() {
        shift_until_any_of(_whitespace_chars);
    }

    //! Jump forward in the stream by n characters
    void shift_by(std::streamsize n) {
        seek_position(position() + n);
    }

    //! Return the current position in the stream
    std::streamsize position() {
        _stream.clear();
        return _stream.tellg();
    }

    //! Jump the the requested position
    void seek_position(std::streamsize pos) {
        _stream.clear();
        _stream.seekg(pos);
        if (_stream.fail())
            throw SizeError("Given position is beyond EOF");
    }

    //! Return true if no more characters can be read from the stream
    bool is_end_of_file() {
        _stream.peek();
        const bool end = _stream.eof();
        _stream.clear();
        return end;
    }

    operator std::istream&() {
        return _stream;
    }

 private:
    std::istream& _stream;
    std::string _whitespace_chars;
};

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ISTREAM_HELPER_HPP_
