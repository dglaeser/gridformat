// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/common/serialization.hpp>
#include <gridformat/compression/zlib.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "zlib_compression_default_opts"_test = [] () {
        GridFormat::Serialization bytes{1000};
        GridFormat::Compression::ZLIB compressor;
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
    };

    "zlib_compression_custom_block_size"_test = [] () {
        GridFormat::Serialization bytes{1000};
        const auto compressor = GridFormat::Compression::ZLIB::with({.block_size = 100});
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.number_of_blocks == 10);
    };

    "zlib_compression_custom_block_size_with_residual"_test = [] () {
        GridFormat::Serialization bytes{1000};
        const auto compressor = GridFormat::Compression::ZLIB::with({.block_size = 300});
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.number_of_blocks == 4);
        expect(block_sizes.residual_block_size == 100);
    };

    return 0;
}
