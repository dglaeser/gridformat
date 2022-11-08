#include <sstream>

#include <boost/ut.hpp>

#include <gridformat/common/writer.hpp>

class MyWriter : public GridFormat::Writer {
 public:
    decltype(auto) get_point_data(const std::string& name) const {
        return GridFormat::Writer::_get_point_data(name);
    }

    decltype(auto) get_cell_data(const std::string& name) const {
        return GridFormat::Writer::_get_cell_data(name);
    }
};

template<typename Field>
void check_streamed_field(const Field& f, const std::string& reference) {
    std::stringstream stream;
    f.stream(stream);

    using boost::ut::expect;
    using boost::ut::eq;
    expect(eq(stream.str(), reference));
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

    "writer_point_data_setter"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_point_data("test", data);
        check_streamed_field(
            writer.get_point_data("test"),
            "1 2 3 4"
        );
    };

    "writer_custom_formatter"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_range_formatter(GridFormat::RangeFormatter{{.delimiter=","}});
        writer.set_point_data("test", data);
        check_streamed_field(
            writer.get_point_data("test"),
            "1,2,3,4"
        );
    };

    "writer_point_data_set_by_reference"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_point_data("test", data);
        data[2] = 42;
        check_streamed_field(
            writer.get_point_data("test"),
            "1 2 42 4"
        );
    };

    "writer_point_data_set_as_view"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_point_data("test", std::ranges::ref_view(data));
        data[2] = 42;
        check_streamed_field(
            writer.get_point_data("test"),
            "1 2 42 4"
        );
    };

    "writer_point_data_set_with_custom_precision"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_point_data("test", data, GridFormat::float64);
        data[2] = 42;
        check_serialization(
            writer.get_point_data("test").serialized(),
            std::vector<double>{1., 2., 42., 4.}
        );
    };

    "writer_cell_data_setter"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_cell_data("test", data);
        check_streamed_field(
            writer.get_cell_data("test"),
            "1 2 3 4"
        );
    };

    "writer_cell_data_set_by_reference"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_cell_data("test", data);
        data[2] = 42;
        check_streamed_field(
            writer.get_cell_data("test"),
            "1 2 42 4"
        );
    };

    "writer_cell_data_set_as_view"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_cell_data("test", std::ranges::ref_view(data));
        data[2] = 42;
        check_streamed_field(
            writer.get_cell_data("test"),
            "1 2 42 4"
        );
    };

    "writer_cell_data_set_with_custom_precision"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_cell_data("test", data, GridFormat::float64);
        data[2] = 42;
        check_serialization(
            writer.get_cell_data("test").serialized(),
            std::vector<double>{1., 2., 42., 4.}
        );
    };

    "writer_vector_point_data"_test = [] () {
        std::vector<std::vector<int>> data{{0, 1}, {2, 3}, {4, 5}, {6, 7}};
        MyWriter writer;
        writer.set_point_data("test", data, GridFormat::float64);
        check_serialization(
            writer.get_point_data("test").serialized(),
            std::vector<double>{0., 1., 2., 3., 4., 5., 6., 7.}
        );
    };

    "writer_vector_cell_data"_test = [] () {
        std::vector<std::vector<int>> data{{0, 1}, {2, 3}, {4, 5}, {6, 7}};
        MyWriter writer;
        writer.set_cell_data("test", data, GridFormat::float64);
        check_serialization(
            writer.get_cell_data("test").serialized(),
            std::vector<double>{0., 1., 2., 3., 4., 5., 6., 7.}
        );
    };

    "writer_tensor_point_data"_test = [] () {
        std::vector<std::vector<std::vector<int>>> data{
            {{0, 1}, {2, 3}},
            {{4, 5}, {6, 7}}
        };
        MyWriter writer;
        writer.set_point_data("test", data, GridFormat::float64);
        check_serialization(
            writer.get_point_data("test").serialized(),
            std::vector<double>{0., 1., 2., 3., 4., 5., 6., 7.}
        );
    };

    "writer_tensor_cell_data"_test = [] () {
        std::vector<std::vector<std::vector<int>>> data{
            {{0, 1}, {2, 3}},
            {{4, 5}, {6, 7}}
        };
        MyWriter writer;
        writer.set_cell_data("test", data, GridFormat::float64);
        check_serialization(
            writer.get_cell_data("test").serialized(),
            std::vector<double>{0., 1., 2., 3., 4., 5., 6., 7.}
        );
    };

    return 0;
}