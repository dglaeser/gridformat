// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>

#include <gridformat/common/serialization.hpp>
#include <gridformat/compression/lzma.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "lzma_compression_default_opts"_test = [] () {
        GridFormat::Serialization bytes{1000};
        GridFormat::Compression::LZMA compressor;
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
    };

    "lzma_compression_custom_block_size"_test = [] () {
        GridFormat::Serialization bytes{1000};
        const auto compressor = GridFormat::Compression::LZMA::with({.block_size = 100});
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.number_of_blocks == 10);
    };

    "lzma_compression_custom_block_size_with_residual"_test = [] () {
        GridFormat::Serialization bytes{1000};
        const auto compressor = GridFormat::Compression::LZMA::with({.block_size = 300});
        const auto block_sizes = compressor.compress(bytes);
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.number_of_blocks == 4);
        expect(block_sizes.residual_block_size == 100);
    };

    "lzma_decompression_default"_test = [] () {
        const std::vector<int> data{42, 43, 44, 45, 56, 66};
        const auto number_of_bytes = data.size()*sizeof(int);

        GridFormat::Serialization bytes{number_of_bytes};
        std::ranges::for_each(
            bytes.template as_span_of<int>(),
            [&, i=int{0}] (int& value) mutable { value = data[i++]; }
        );

        GridFormat::Compression::LZMA compressor;
        const auto blocks = compressor.compress(bytes);
        compressor.decompress(bytes, blocks);
        expect(eq(bytes.size(), number_of_bytes));
        expect(std::ranges::equal(bytes.template as_span_of<int>(), data));
    };

    "lzma_decompression_multiple_blocks"_test = [] () {
        const std::vector<int> data{42, 43, 44, 45, 56, 66};
        const auto number_of_bytes = data.size()*sizeof(int);

        GridFormat::Serialization bytes{number_of_bytes};
        std::ranges::for_each(
            bytes.template as_span_of<int>(),
            [&, i=int{0}] (int& value) mutable { value = data[i++]; }
        );

        GridFormat::Compression::LZMA compressor{{.block_size = number_of_bytes/3}};
        const auto blocks = compressor.compress(bytes);
        compressor.decompress(bytes, blocks);
        expect(eq(bytes.size(), number_of_bytes));
        expect(std::ranges::equal(bytes.template as_span_of<int>(), data));
    };

    return 0;
}
