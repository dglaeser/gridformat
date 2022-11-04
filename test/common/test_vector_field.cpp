#include <iostream>
#include <vector>
#include <ranges>
#include <sstream>

#include <boost/ut.hpp>

#include <gridformat/common/fields.hpp>

template<typename Field>
void check_streamed_field(const Field& f, const std::string& reference) {
    std::stringstream stream;
    f.stream(stream);

    using boost::ut::expect;
    using boost::ut::eq;
    expect(eq(stream.str(), reference));
}

template<typename T, typename Field>
void check_field_precision(const Field& f) {
    using boost::ut::expect;
    expect(GridFormat::as_dynamic(GridFormat::Precision<T>{}) == f.precision());
}

template<typename Serialization, std::ranges::range R>
void check_serialization(const Serialization& serialization, const R& reference) {
    using boost::ut::expect;
    using boost::ut::eq;
    using T = std::ranges::range_value_t<R>;
    const auto reference_size = std::ranges::distance(reference);
    expect(eq(serialization.size(), reference_size*sizeof(T)));

    const T* serialized_data = reinterpret_cast<const T*>(serialization.data());
    int i = 0;
    for (auto ref : reference)
        expect(eq(ref, serialized_data[i++]));
}

int main() {

    using namespace boost::ut;
    using namespace boost::ut::literals;

    "vector_field_stream"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        GridFormat::VectorField field{std::ranges::ref_view{data}};
        check_streamed_field(field, "1 2 3 4");
        expect(eq(field.number_of_components(), 2));
        check_field_precision<int>(field);
    };

    "vector_field_custom_delimiter_stream"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        GridFormat::VectorField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{.delimiter = ","}}
        };
        check_streamed_field(field, "1,2,3,4");
        expect(eq(field.number_of_components(), 2));
        check_field_precision<int>(field);
    };

    "vector_field_custom_prefix_stream"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        GridFormat::VectorField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{
                .delimiter = ",",
                .line_prefix = "P"
            }}
        };
        check_streamed_field(field, "P1,2,3,4");
        expect(eq(field.number_of_components(), 2));
        check_field_precision<int>(field);
    };

    "vector_field_custom_number_of_line_entries_stream"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2, 3}, {4, 5, 6}};
        GridFormat::VectorField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{
                .delimiter = ",",
                .line_prefix = "P",
                .num_entries_per_line = 3
            }}
        };
        check_streamed_field(field, "P1,2,3\nP4,5,6");
        expect(eq(field.number_of_components(), 3));
        check_field_precision<int>(field);
    };

    "vector_field_serialization"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        GridFormat::VectorField field{std::ranges::ref_view(data)};
        check_serialization(field.serialized(), std::views::join(data));
        check_field_precision<int>(field);
    };

    "vector_field_custom_precision"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2, 3}, {4, 5, 6}};
        GridFormat::VectorField field{data | std::views::all, GridFormat::Precision<double>{}};
        check_serialization(field.serialized(), std::vector<double>{1, 2, 3, 4, 5, 6});
        check_field_precision<double>(field);
    };

    return 0;
}