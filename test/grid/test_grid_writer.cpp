// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <sstream>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <vector>

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
    : ParentType(grid, "", GridFormat::WriterOptions{false, false})
    {}

    decltype(auto) get_point_field(const std::string& name) const {
        return ParentType::_get_point_field(name);
    }

    decltype(auto) get_cell_field(const std::string& name) const {
        return ParentType::_get_cell_field(name);
    }

 private:
    void _write(std::ostream&) const override {
        throw GridFormat::InvalidState("This test should not call _write()");
    }
};

template<typename T = int>
class MyField : public GridFormat::Field {
 public:
    MyField(std::vector<int> values, const GridFormat::Precision<T>& = {})
    : _values(std::move(values))
    {}

 private:
    std::vector<int> _values;

    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{{_values.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<T>{}};
    }

    typename GridFormat::Serialization _serialized() const override {
        typename GridFormat::Serialization result(_values.size()*sizeof(T));
        auto out_data = result.template as_span_of<T>();
        for (std::size_t i = 0; i < _values.size(); ++i)
            out_data[i] = static_cast<T>(_values[i]);
        return result;
    }
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

template<typename T = int, std::ranges::forward_range Entities>
auto make_values(const Entities& entities) {
    std::vector<T> values(std::ranges::distance(entities));
    std::ranges::for_each(entities, [&] (const auto& e) {
        values[e.id] = 42 + e.id;
    });
    return values;
}

template<std::ranges::forward_range Entities, typename T>
auto make_sorted_by_entities(const Entities& entities, const std::vector<T>& data) {
    if (static_cast<std::size_t>(std::ranges::distance(entities)) != data.size())
        throw GridFormat::SizeError("Entity range - size mismatch");
    auto cpy = data;
    std::size_t i = 0;
    std::ranges::for_each(entities, [&] (const auto& e) {
        cpy[i++] = data[e.id];
    });
    return cpy;
}

template<typename T = int, std::ranges::forward_range Entities>
auto make_sorted_by_entities(const Entities& entities) {
    auto values = make_values<T>(entities);
    return make_sorted_by_entities(entities, values);
}

template<typename T = int, typename Grid>
auto make_point_values(const Grid& grid) {
    return make_values<T>(GridFormat::points(grid));
}

template<typename T = int, typename Grid>
auto make_cell_values(const Grid& grid) {
    return make_values<T>(GridFormat::cells(grid));
}

template<typename T = int, typename Grid>
auto make_point_values_sorted_by_grid(const Grid& grid) {
    return make_sorted_by_entities<T>(GridFormat::points(grid));
}

template<typename Grid, typename T>
auto make_point_values_sorted_by_grid(const Grid& grid, const std::vector<T>& data) {
    return make_sorted_by_entities(GridFormat::points(grid), data);
}

template<typename T = int, typename Grid>
auto make_cell_values_sorted_by_grid(const Grid& grid) {
    return make_sorted_by_entities<T>(GridFormat::cells(grid));
}

template<typename Grid, typename T>
auto make_cell_values_sorted_by_grid(const Grid& grid, const std::vector<T>& data) {
    return make_sorted_by_entities(GridFormat::cells(grid), data);
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::expect;

    const auto grid = GridFormat::Test::make_unstructured_2d();

    "grid_writer_point_field"_test = [&] () {
        MyWriter writer{grid};
        const auto field_values = make_point_values(grid);
        writer.set_point_field("test", [&] (const auto& point) {
            return field_values[point.id];
        });
        check_serialization(
            writer.get_point_field("test"),
            make_point_values_sorted_by_grid(grid)
        );
    };

    "grid_writer_point_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        const auto field_values = make_point_values(grid);
        writer.set_point_field("test", [&] (const auto& point) {
            return field_values[point.id];
        }, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_point_field("test"),
            make_point_values_sorted_by_grid<double>(grid)
        );
    };

    "grid_writer_cell_field"_test = [&] () {
        MyWriter writer{grid};
        const auto field_values = make_cell_values(grid);
        writer.set_cell_field("test", [&] (const auto& cell) {
            return field_values[cell.id];
        });
        check_serialization(
            writer.get_cell_field("test"),
            make_cell_values_sorted_by_grid(grid)
        );
    };

    "grid_writer_cell_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        const auto field_values = make_cell_values(grid);
        writer.set_cell_field("test", [&] (const auto& cell) {
            return field_values[cell.id];
        }, GridFormat::Precision<double>{});
        check_serialization(
            writer.get_cell_field("test"),
            make_cell_values_sorted_by_grid<double>(grid)
        );
    };

    "grid_writer_values_by_reference"_test = [&] () {
        MyWriter writer{grid};
        auto point_values = make_point_values(grid);
        auto cell_values = make_cell_values(grid);
        writer.set_point_field("test", [&] (const auto& point) {
            return point_values[point.id];
        });
        writer.set_cell_field("test", [&] (const auto& cell) {
            return cell_values[cell.id];
        });
        point_values[1] = 99;
        check_serialization(
            writer.get_point_field("test"),
            make_point_values_sorted_by_grid(grid, point_values)
        );
        check_serialization(
            writer.get_cell_field("test"),
            make_cell_values_sorted_by_grid(grid, cell_values)
        );
    };

    "writer_set_custom_point_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_point_field("test", MyField{make_point_values(grid)});
        check_serialization(
            writer.get_point_field("test"),
            make_point_values(grid)
        );
    };

    "writer_set_custom_cell_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_cell_field("test", MyField{make_cell_values(grid)});
        check_serialization(
            writer.get_cell_field("test"),
            make_cell_values(grid)
        );
    };

    "writer_set_custom_point_field_custom_precision"_test = [&] () {
        MyWriter writer{grid};
        writer.set_point_field(
            "test",
            MyField{
                make_point_values(grid),
                GridFormat::Precision<double>{}
            }
        );
        check_serialization(
            writer.get_point_field("test"),
            make_point_values<double>(grid)
        );
    };

    "writer_set_custom_cell_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_cell_field(
            "test",
            MyField{
                make_cell_values(grid),
                GridFormat::Precision<double>{}
            }
        );
        check_serialization(
            writer.get_cell_field("test"),
            make_cell_values<double>(grid)
        );
    };

    "grid_writer_remove_point_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_point_field("test", [&] (const auto&) { return 1.0; });
        auto field = writer.remove_point_field("test");
        expect(throws([&] () { writer.remove_point_field("test"); }));
    };

    "grid_writer_remove_cell_field"_test = [&] () {
        MyWriter writer{grid};
        writer.set_cell_field("test", [&] (const auto&) { return 1.0; });
        auto field = writer.remove_cell_field("test");
        expect(throws([&] () { writer.remove_cell_field("test"); }));
    };

    "grid_writer_remove_meta_data"_test = [&] () {
        MyWriter writer{grid};
        writer.set_meta_data("test", 1.0);
        auto field = writer.remove_meta_data("test");
        expect(throws([&] () { writer.remove_meta_data("test"); }));
    };

    "grid_writer_get_fields_of_rank"_test = [&] () {
        std::array vector{1., 1.};
        std::array tensor{vector, vector};

        const auto set_fields = [&] (const auto& setter) {
            setter("scalar0", [&] (const auto&) { return 1.0; });
            setter("scalar1", [&] (const auto&) { return 1.0; });
            setter("vector0", [&] (const auto&) { return vector; });
            setter("vector1", [&] (const auto&) { return vector; });
            setter("tensor0", [&] (const auto&) { return tensor; });
            setter("tensor1", [&] (const auto&) { return tensor; });
        };

        MyWriter writer{grid};
        set_fields([&] <typename... Args> (Args&&... args) { writer.set_point_field(std::forward<Args>(args)...); });
        set_fields([&] <typename... Args> (Args&&... args) { writer.set_cell_field(std::forward<Args>(args)...); });

        const auto matches = [] (auto pairs, std::vector<std::string> expected) {
            for (const auto& [n, _] : pairs)
                if (std::ranges::count(expected, n))
                    std::erase(expected, n);
            return expected.empty();
        };

        expect(matches(point_fields_of_rank(0, writer), {"scalar0", "scalar1"}));
        expect(matches(cell_fields_of_rank(0, writer), {"scalar0", "scalar1"}));

        expect(matches(point_fields_of_rank(1, writer), {"vector0", "vector1"}));
        expect(matches(cell_fields_of_rank(1, writer), {"vector0", "vector1"}));

        expect(matches(point_fields_of_rank(2, writer), {"tensor0", "tensor1"}));
        expect(matches(cell_fields_of_rank(2, writer), {"tensor0", "tensor1"}));
    };

    return 0;
}
