#include <boost/ut.hpp>

#include <gridformat/common/serialization.hpp>
#include <gridformat/compression/lzma.hpp>

int main() {
    using namespace boost::ut;

    "lzma_compression_default_opts"_test = [] () {
        GridFormat::Serialization bytes(1000);
        GridFormat::Compression::LZMA compressor;
        const auto block_sizes = compressor.compress(bytes.data(), bytes.size());
        expect(block_sizes.compressed_size() <= 1000);
    };

    "lzma_compression_custom_block_size"_test = [] () {
        GridFormat::Serialization bytes(1000);
        const auto compressor = GridFormat::Compression::lzma_with({.block_size = 100});
        const auto block_sizes = compressor.compress(bytes.data(), bytes.size());
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.num_blocks() == 10);
    };

    "lzma_compression_custom_block_size_with_residual"_test = [] () {
        GridFormat::Serialization bytes(1000);
        const auto compressor = GridFormat::Compression::lzma_with({.block_size = 300});
        const auto block_sizes = compressor.compress(bytes.data(), bytes.size());
        expect(block_sizes.compressed_size() <= 1000);
        expect(block_sizes.num_blocks() == 4);
        expect(block_sizes.residual_block_size() == 100);
    };

    return 0;
}