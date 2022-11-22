#include <vector>
#include <sstream>
#include <iterator>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/grid/writer.hpp>
#include "unstructured_grid.hpp"
#include "../testing.hpp"

template<typename Grid>
class MyWriter : public GridFormat::GridWriter<Grid> {
    using ParentType = GridFormat::GridWriter<Grid>;

 public:
    explicit MyWriter(const Grid& grid)
    : ParentType(grid, "")
    {}

    decltype(auto) get_point_field(const std::string& name) const {
        return ParentType::_get_point_field(name);
    }

    decltype(auto) get_cell_field(const std::string& name) const {
        return ParentType::_get_cell_field(name);
    }

 private:
    void _write(std::ostream&) const override {
        throw GridFormat::NotImplemented("_write()");
    }
};

template<typename T = int>
class MyField : public GridFormat::Field {
 public:
    MyField(std::vector<int> values, const GridFormat::Precision<T>& = {})
    : _values(std::move(values))
    {}

    int get(std::size_t i) const {
        if (i >= _values.size())
            throw GridFormat::SizeError("Given index exceeds field size");
        return _values[i];
    }

 private:
    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{std::vector{_values.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<T>{}};
    }

    typename GridFormat::Serialization _serialized() const override {
        typename GridFormat::Serialization result(_values.size()*sizeof(T));
        T* data = reinterpret_cast<T*>(result.as_span().data());
        for (std::size_t i = 0; i < _values.size(); ++i)
            data[i] = static_cast<T>(_values[i]);
        return result;
    }

    std::vector<int> _values;
};

template<typename T>
void check_serialization(const GridFormat::Field& field, const std::vector<T>& reference) {
    auto serialization = field.serialized();
    if (field.precision().is_signed() != std::is_signed_v<T>)
        throw GridFormat::SizeError("Precision (signedness) does not match reference");
    if (field.precision().is_integral() != std::is_integral_v<T>)
        throw GridFormat::SizeError("Precision (is_integral) does not match reference");
    if (field.precision().size_in_bytes() != sizeof(T))
        throw GridFormat::SizeError("Precision (byte size) does not match reference");
    if (serialization.size() != reference.size()*sizeof(T))
        throw GridFormat::SizeError("Serialization size does not match the reference");

    using GridFormat::Testing::eq;
    using GridFormat::Testing::expect;
    const T* values = reinterpret_cast<const T*>(serialization.as_span().data());
    for (std::size_t i = 0; i < reference.size(); ++i)
        expect(eq(values[i], reference[i]));
}

int main() {
    using GridFormat::Testing::operator""_test;

    const auto grid = GridFormat::Test::make_unstructured_2d();

    "grid_writer_point_field"_test = [&] () {
        MyWriter writer{grid};
        std::vector<int> field_values{42, 43, 44, 45, 46};
        writer.set_point_field("test", [&] (const auto& point) {
            return field_values[point.id];
        });
        check_serialization(writer.get_point_field("test"), field_values);
    };

    "grid_writer_point_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        std::vector<int> field_values{42, 43, 44, 45, 46};
        writer.set_point_field("test", [&] (const auto& point) {
            return field_values[point.id];
        }, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_point_field("test"),
            std::vector<double>{42, 43, 44, 45, 46}
        );
    };

    "grid_writer_cell_field"_test = [&] () {
        MyWriter writer{grid};
        std::vector<int> field_values{42, 43};
        writer.set_cell_field("test", [&] (const auto& cell) {
            return field_values[cell.id];
        });
        check_serialization(writer.get_cell_field("test"), field_values);
    };

    "grid_writer_cell_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        std::vector<int> field_values{42, 43};
        writer.set_cell_field("test", [&] (const auto& cell) {
            return field_values[cell.id];
        }, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_cell_field("test"),
            std::vector<double>{42, 43}
        );
    };

    "grid_writer_values_by_reference"_test = [&] () {
        MyWriter writer{grid};
        std::vector<int> field_values{42, 43, 44, 45, 46};
        writer.set_point_field("test", [&] (const auto& point) {
            return field_values[point.id];
        });
        writer.set_cell_field("test", [&] (const auto& cell) {
            return field_values[cell.id];
        });
        field_values[1] = 99;
        check_serialization(
            writer.get_point_field("test"),
            std::vector<int>{42, 99, 44, 45, 46}
        );
        check_serialization(
            writer.get_cell_field("test"),
            std::vector<int>{42, 99}
        );
    };

    "writer_set_custom_point_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_point_field("test", MyField{std::vector<int>{42, 43, 44, 45, 46}});
        check_serialization(
            writer.get_point_field("test"),
            std::vector<int>{42, 43, 44, 45, 46}
        );
    };

    "writer_set_custom_cell_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_cell_field("test", MyField{std::vector<int>{42, 43}});
        check_serialization(
            writer.get_cell_field("test"),
            std::vector<int>{42, 43}
        );
    };

    "writer_set_custom_point_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        writer.set_point_field(
            "test",
            MyField{
                std::vector<int>{42, 43, 44, 45, 46},
                GridFormat::Precision<double>{}
            }
        );
        check_serialization(
            writer.get_point_field("test"),
            std::vector<double>{42, 43, 44, 45, 46}
        );
    };

    "writer_set_custom_cell_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_cell_field(
            "test",
            MyField{
                std::vector<int>{42, 43},
                GridFormat::Precision<double>{}
            }
        );
        check_serialization(
            writer.get_cell_field("test"),
            std::vector<double>{42, 43}
        );
    };

    return 0;
}