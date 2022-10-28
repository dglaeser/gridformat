#include <iostream>
#include <vector>
#include <ranges>

#include <boost/ut.hpp>

#include <gridformat/common/tensor_field.hpp>

int main() {

    using namespace boost::ut;
    using namespace boost::ut::literals;
    "scalar_field_type"_test = [] () {
        std::vector<double> data{1.0, 2.0, 3.0, 4.0};
        GridFormat::TensorField field{std::ranges::ref_view{data}};
        expect(field.type().is_scalar());
    };
    "vector_field_type"_test = [] () {
        std::vector<std::vector<int>> data{
            std::vector<int>{1},
            std::vector<int>{2},
            std::vector<int>{3},
            std::vector<int>{4}
        };
        GridFormat::TensorField field{std::ranges::ref_view{data}};
        expect(field.type().is_scalar());
        // expect(eq(field.type().dimensions()[0], std::size_t{1}));
    };

    return 0;
}