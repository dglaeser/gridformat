#include <boost/ut.hpp>

#include <gridformat/common/range_field.hpp>

int main() {
    using namespace boost::ut;

    "range_field_by_value"_test = [] () {
        GridFormat::RangeField field{std::vector<int>{1, 2, 3, 4}};
        expect(eq(field.layout().dimension(), std::size_t{1}));
        expect(eq(field.layout().extent(0), std::size_t{4}));

        const auto serialization = field.serialized();
        expect(eq(serialization.size(), std::size_t{4*sizeof(int)}));

        const int* data = reinterpret_cast<const int*>(serialization.data());
        expect(eq(data[0], 1));
        expect(eq(data[1], 2));
        expect(eq(data[2], 3));
        expect(eq(data[3], 4));
    };

    "range_field_custom_value_type_by_value"_test = [] () {
        GridFormat::RangeField field{
            std::vector<int>{1, 2, 3, 4},
            GridFormat::Precision<double>{}
        };
        expect(eq(field.layout().dimension(), std::size_t{1}));
        expect(eq(field.layout().extent(0), std::size_t{4}));

        const auto serialization = field.serialized();
        expect(eq(serialization.size(), std::size_t{4*sizeof(double)}));

        const double* data = reinterpret_cast<const double*>(serialization.data());
        expect(eq(data[0], 1.));
        expect(eq(data[1], 2.));
        expect(eq(data[2], 3.));
        expect(eq(data[3], 4.));
    };

    "range_field_vector_by_reference"_test = [] () {
        std::vector<std::vector<int>> field_data {{1, 2}, {3, 4}};
        GridFormat::RangeField field{field_data};
        expect(eq(field.layout().dimension(), std::size_t{2}));
        expect(eq(field.layout().extent(0), std::size_t{2}));
        expect(eq(field.layout().extent(1), std::size_t{2}));
        expect(eq(field.layout().number_of_entries(), std::size_t{4}));

        field_data[0][1] = 42;
        const auto serialization = field.serialized();
        expect(eq(serialization.size(), std::size_t{4*sizeof(int)}));

        const int* data = reinterpret_cast<const int*>(serialization.data());
        expect(eq(data[0], 1));
        expect(eq(data[1], 42));
        expect(eq(data[2], 3));
        expect(eq(data[3], 4));
    };

    "range_field_tensor_by_reference_custom_precision"_test = [] () {
        std::vector<std::vector<std::vector<int>>> field_data {
            {{1, 2, 3}, {4, 5, 6}}
        };
        GridFormat::RangeField field{field_data, GridFormat::float64};
        expect(eq(field.layout().dimension(), std::size_t{3}));
        expect(eq(field.layout().extent(0), std::size_t{1}));
        expect(eq(field.layout().extent(1), std::size_t{2}));
        expect(eq(field.layout().extent(2), std::size_t{3}));
        expect(eq(field.layout().number_of_entries(), std::size_t{6}));

        field_data[0][1][0] = 42;
        const auto serialization = field.serialized();
        expect(eq(serialization.size(), std::size_t{6*sizeof(double)}));

        const double* data = reinterpret_cast<const double*>(serialization.data());
        expect(eq(data[0], 1.));
        expect(eq(data[1], 2.));
        expect(eq(data[2], 3.));
        expect(eq(data[3], 42.));
        expect(eq(data[4], 5.));
        expect(eq(data[5], 6.));
    };

    return 0;
}