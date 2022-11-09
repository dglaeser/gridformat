#include <sstream>

#include <boost/ut.hpp>

#include <gridformat/common/writer.hpp>

class MyWriter : public GridFormat::Writer {
 public:
    decltype(auto) get_point_field(const std::string& name) const {
        return GridFormat::Writer::_get_point_field(name);
    }

    decltype(auto) get_cell_field(const std::string& name) const {
        return GridFormat::Writer::_get_cell_field(name);
    }
};

class MyField : public GridFormat::Field {
 public:
    MyField() : GridFormat::Field(
        GridFormat::MDLayout{std::vector<int>{3}},
        GridFormat::Precision<int>{}
    ) {}

 private:
    typename GridFormat::Field::Serialization _serialized() const override {
        typename GridFormat::Field::Serialization result(3*sizeof(int));
        int* data = reinterpret_cast<int*>(result.data());
        std::ranges::copy(_values, data);
        return result;
    }

    std::array<int, 3> _values{42, 43, 44};
};

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

    "writer_point_data_by_reference"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_point_field("test", data);

        data[2] = 42;
        check_serialization(
            writer.get_point_field("test").serialized(),
            data
        );
    };

    "writer_cell_data_by_reference"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        MyWriter writer;
        writer.set_cell_field("test", data);

        data[2] = 42;
        check_serialization(
            writer.get_cell_field("test").serialized(),
            data
        );
    };

    "writer_point_data_by_value"_test = [] () {
        MyWriter writer;
        writer.set_point_field("test", std::vector<int>{1, 2, 3, 4});
        check_serialization(
            writer.get_point_field("test").serialized(),
            std::vector<int>{1, 2, 3, 4}
        );
    };

    "writer_cell_data_by_value"_test = [] () {
        MyWriter writer;
        writer.set_cell_field("test", std::vector<int>{1, 2, 3, 4});
        check_serialization(
            writer.get_cell_field("test").serialized(),
            std::vector<int>{1, 2, 3, 4}
        );
    };

    "writer_point_data_by_reference_custom_precision"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        MyWriter writer;
        writer.set_point_field("test", data, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_point_field("test").serialized(),
            std::vector<double>{1, 2, 3, 4}
        );
    };

    "writer_cell_data_by_reference_custom_precision"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        MyWriter writer;
        writer.set_cell_field("test", data, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_cell_field("test").serialized(),
            std::vector<double>{1, 2, 3, 4}
        );
    };

    "writer_point_data_by_value_custom_precision"_test = [] () {
        MyWriter writer;
        writer.set_point_field("test", std::vector<int>{1, 2, 3, 4}, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_point_field("test").serialized(),
            std::vector<double>{1, 2, 3, 4}
        );
    };

    "writer_cell_data_by_value_custom_precision"_test = [] () {
        MyWriter writer;
        writer.set_cell_field("test", std::vector<int>{1, 2, 3, 4}, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_cell_field("test").serialized(),
            std::vector<double>{1, 2, 3, 4}
        );
    };

    "writer_point_data_as_view_custom_precision"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        MyWriter writer;
        writer.set_point_field(
            "test",
            data | std::views::transform([] (const auto& v) {
                return std::array<int, 3>{v[0], v[1], 0};
            }),
            GridFormat::Precision<double>{}
        );
        check_serialization(
            writer.get_point_field("test").serialized(),
            std::vector<double>{1, 2, 0, 3, 4, 0}
        );
    };

    "writer_cell_data_as_view_custom_precision"_test = [] () {
        std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
        MyWriter writer;
        writer.set_cell_field(
            "test",
            data | std::views::transform([] (const auto& v) {
                return std::array<int, 3>{v[0], v[1], 0};
            }),
            GridFormat::Precision<double>{}
        );
        check_serialization(
            writer.get_cell_field("test").serialized(),
            std::vector<double>{1, 2, 0, 3, 4, 0}
        );
    };

    "writer_set_custom_point_field"_test = [] () {
        MyWriter writer;
        writer.set_point_field("test", MyField{});
        check_serialization(
            writer.get_point_field("test").serialized(),
            std::vector<int>{42, 43, 44}
        );
    };

    "writer_set_custom_cell_field"_test = [] () {
        MyWriter writer;
        writer.set_cell_field("test", MyField{});
        check_serialization(
            writer.get_cell_field("test").serialized(),
            std::vector<int>{42, 43, 44}
        );
    };

    return 0;
}