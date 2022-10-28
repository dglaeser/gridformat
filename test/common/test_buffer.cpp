#include <boost/ut.hpp>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/buffer.hpp>

int main() {
    constexpr std::size_t size = 20;
    std::vector<int> data(size, 42.0);
    GridFormat::Buffer buffer(data.data(), size);
    GridFormat::OwningBuffer<int> owning(size);
    owning.fill(data);

    static_assert(GridFormat::Concepts::Buffer<GridFormat::Buffer<int>>);
    static_assert(GridFormat::Concepts::Buffer<GridFormat::OwningBuffer<int>>);
    static_assert(GridFormat::Concepts::Buffer<std::vector<int>>);

    using namespace boost::ut;
    "buffer"_test = [&] { boost::ut::expect(std::ranges::equal(buffer, data)); };
    "owning_buffer"_test = [&] { boost::ut::expect(std::ranges::equal(owning, data)); };

    return 0;
}