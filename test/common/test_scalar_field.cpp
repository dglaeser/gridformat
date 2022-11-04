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
    expect(eq(f.number_of_components(), 1));
}

template<typename T, typename Field>
void check_field_precision(const Field& f) {
    using boost::ut::expect;
    expect(GridFormat::as_dynamic(GridFormat::Precision<T>{}) == f.precision());
}

template<typename Serialization, typename T>
void check_serialization(const Serialization& serialization,
                         const std::vector<T>& reference) {
    using boost::ut::expect;
    using boost::ut::eq;
    expect(eq(serialization.size(), reference.size()*sizeof(T)));

    const T* serialized_data = reinterpret_cast<const T*>(serialization.data());
    for (std::size_t i = 0; i < reference.size(); ++i)
            expect(eq(reference[i], serialized_data[i]));
}

int main() {

    using namespace boost::ut;
    using namespace boost::ut::literals;

    "scalar_field_stream"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{std::ranges::ref_view{data}};
        check_streamed_field(field, "1 2 3 4");
        check_field_precision<int>(field);
    };

    "scalar_field_custom_delimiter_stream"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{.delimiter = ","}}
        };
        check_streamed_field(field, "1,2,3,4");
        check_field_precision<int>(field);
    };

    "scalar_field_custom_prefix_stream"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{
                .delimiter = ",",
                .line_prefix = "P"
            }}
        };
        check_streamed_field(field, "P1,2,3,4");
        check_field_precision<int>(field);
    };

    "scalar_field_custom_number_of_line_entries_stream"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{
            std::ranges::ref_view{data},
            GridFormat::RangeFormatter{{
                .delimiter = ",",
                .line_prefix = "P",
                .num_entries_per_line = 2
            }}
        };
        check_streamed_field(field, "P1,2\nP3,4");
        check_field_precision<int>(field);
    };

    "scalar_field_serialization"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{std::ranges::ref_view(data)};
        check_serialization(field.serialized(), data);
        check_field_precision<int>(field);
    };

    "scalar_field_converted_serialization"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{
            std::views::transform(data, [] (int v) { return static_cast<double>(v); })
        };
        check_serialization(field.serialized(), std::vector<double>{1, 2, 3, 4});
        check_field_precision<double>(field);
    };

    "scalar_field_custom_precision"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        GridFormat::ScalarField field{data | std::views::all, GridFormat::Precision<double>{}};
        check_serialization(field.serialized(), std::vector<double>{1, 2, 3, 4});
        check_field_precision<double>(field);
    };

    return 0;
}