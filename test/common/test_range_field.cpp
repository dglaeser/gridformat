#include <vector>

#include <gridformat/common/field.hpp>
#include <gridformat/common/range_field.hpp>
#include <gridformat/common/exceptions.hpp>

#include "../testing.hpp"

template<typename T, typename Expected = T>
class Tester {
 public:
    explicit Tester(std::vector<T>&& reference)
    : _reference(std::move(reference))
    {}

    void test(const GridFormat::Field& field) const {
        const auto serialization = field.serialized();
        field.precision().visit([&] <typename _T> (const GridFormat::Precision<_T>&) {
            const auto field_vals = serialization.template as_span_of<Expected>();

            if (!std::is_same_v<_T, Expected>)
                throw GridFormat::TypeError("Unexpected field precision");
            if (serialization.size()/sizeof(Expected) != _reference.size())
                throw GridFormat::SizeError("Field size mismatch");
            if (_reference.size() != field.layout().number_of_entries())
                throw GridFormat::SizeError("Field size mismatch");
            if (_reference.size() != field_vals.size())
                throw GridFormat::SizeError("Field size mismatch");

            for (std::size_t i = 0; i < _reference.size(); ++i)
                GridFormat::Testing::expect(
                    GridFormat::Testing::eq(_reference[i], field_vals[i])
                );
        });
    }

 private:
    std::vector<T> _reference;
};

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "range_field_by_value"_test = [] () {
        GridFormat::RangeField field{std::vector<int>{1, 2, 3, 4}};

        const auto layout = field.layout();
        expect(eq(layout.dimension(), std::size_t{1}));
        expect(eq(layout.extent(0), std::size_t{4}));

        Tester tester{std::vector<int>{1, 2, 3, 4}};
        tester.test(field);
    };

    "range_field_custom_value_type_by_value"_test = [] () {
        GridFormat::RangeField field{
            std::vector<int>{1, 2, 3, 4},
            GridFormat::Precision<double>{}
        };

        const auto layout = field.layout();
        expect(eq(layout.dimension(), std::size_t{1}));
        expect(eq(layout.extent(0), std::size_t{4}));

        Tester tester{std::vector<double>{1., 2., 3., 4.}};
        tester.test(field);
    };

    "range_field_vector_by_reference"_test = [] () {
        std::vector<std::vector<int>> field_data {{1, 2}, {3, 4}};
        GridFormat::RangeField field{field_data};

        const auto layout = field.layout();
        expect(eq(layout.dimension(), std::size_t{2}));
        expect(eq(layout.extent(0), std::size_t{2}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.number_of_entries(), std::size_t{4}));

        Tester tester{std::vector<int>{1, 2, 3, 4}};
        tester.test(field);
    };

    "range_field_tensor_by_reference_custom_precision"_test = [] () {
        std::vector<std::vector<std::vector<int>>> field_data {
            {{1, 2, 3}, {4, 5, 6}}
        };
        GridFormat::RangeField field{field_data, GridFormat::float64};

        const auto layout = field.layout();
        expect(eq(layout.dimension(), std::size_t{3}));
        expect(eq(layout.extent(0), std::size_t{1}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.extent(2), std::size_t{3}));
        expect(eq(layout.number_of_entries(), std::size_t{6}));

        field_data[0][1][0] = 42;
        Tester tester{std::vector<double>{1, 2, 3, 42, 5, 6}};
        tester.test(field);
    };

    return 0;
}
