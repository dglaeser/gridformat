#include <boost/ut.hpp>

#include <gridformat/common/md_layout.hpp>

int main() {
    using namespace boost::ut;

    "md_layout"_test = [] () {
        GridFormat::MDLayout layout{std::vector<int>{1, 2, 3}};
        expect(eq(layout.number_of_entries(), std::size_t{6}));
        expect(eq(layout.extent(0), std::size_t{1}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.extent(2), std::size_t{3}));
        expect(eq(layout.dimension(), std::size_t{3}));
    };

    "md_layout_scalar"_test = [] () {
        const auto layout = GridFormat::get_layout(double{});
        expect(eq(layout.number_of_entries(), std::size_t{1}));
        expect(eq(layout.dimension(), std::size_t{1}));
    };

    "md_layout_vector"_test = [] () {
        std::vector<std::array<double, 2>> vector(3);
        const auto layout = GridFormat::get_layout(vector);
        expect(eq(layout.dimension(), std::size_t{2}));
        expect(eq(layout.extent(0), std::size_t{3}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.number_of_entries(), std::size_t{6}));
    };

    "md_layout_tensor"_test = [] () {
        std::vector<std::array<std::array<double, 2>, 4>> tensor(3);
        const auto layout = GridFormat::get_layout(tensor);
        expect(eq(layout.dimension(), std::size_t{3}));
        expect(eq(layout.extent(0), std::size_t{3}));
        expect(eq(layout.extent(1), std::size_t{4}));
        expect(eq(layout.extent(2), std::size_t{2}));
        expect(eq(layout.number_of_entries(), std::size_t{24}));
    };

    return 0;
}