// SPDX-FileCopyrightText: 2025 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <ranges>

#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vtp_reader.hpp>
#include <gridformat/adapters/reader_polylines_subdivider.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"

void test(const unsigned int number_of_subdivisions_per_segment) {
    using namespace GridFormat;
    using Testing::operator""_test;
    using Testing::expect;
    using Testing::eq;

    const auto write_vtp_file = [&] (const auto& grid, const std::string& filename) {
        VTPWriter writer = VTPWriter{grid}.with_encoding(Encoding::ascii);
        return write_test_file<2>(writer, filename);
    };

    const auto grid = Test::make_unstructured_1d_with_polylines<2>(number_of_subdivisions_per_segment);
    const auto vtp_filename = write_vtp_file(grid, "test_polyline_adapter_subdivisions_" + std::to_string(number_of_subdivisions_per_segment));

    VTPReader reader;
    auto adapted_reader = ReaderAdapters::PolylinesSubdivider{GridFormat::VTPReader{}};
    reader.open(vtp_filename);
    adapted_reader.open(vtp_filename);

    "number_of_cells_points"_test = [&] () {
        expect(eq(reader.number_of_cells(), grid.cells().size()));
        expect(eq(adapted_reader.number_of_points(), reader.number_of_points()));
        expect(eq(adapted_reader.number_of_cells(), grid.cells().size()*number_of_subdivisions_per_segment));
    };

    "field_names"_test = [&] () {
        expect(std::ranges::equal(cell_field_names(reader), cell_field_names(adapted_reader)));
        expect(std::ranges::equal(point_field_names(reader), point_field_names(adapted_reader)));
        expect(std::ranges::equal(meta_data_field_names(reader), meta_data_field_names(adapted_reader)));
    };

    "point_field_values"_test = [&] () {
        std::ranges::for_each(point_field_names(reader), [&] (const auto& name) {
            const auto original_field = reader.point_field(name);
            original_field->precision().visit([&] <typename T> (const Precision<T>&) {
                const auto original = original_field->serialized();
                const auto adapted = adapted_reader.point_field(name)->serialized();
                expect(std::ranges::equal(original.template as_span_of<T>(), adapted.template as_span_of<T>()));
            });
        });
    };

    "metadata_field_values"_test = [&] () {
        std::ranges::for_each(meta_data_field_names(reader), [&] (const auto& name) {
            const auto original_field = reader.meta_data_field(name);
            original_field->precision().visit([&] <typename T> (const Precision<T>&) {
                const auto original = original_field->serialized();
                const auto adapted = adapted_reader.meta_data_field(name)->serialized();
                expect(std::ranges::equal(original.template as_span_of<T>(), adapted.template as_span_of<T>()));
            });
        });
    };

    "cell_field_values"_test = [&] () {
        std::ranges::for_each(cell_field_names(reader), [&] (const auto& name) {
            const auto original_field = reader.cell_field(name);
            original_field->precision().visit([&] <typename T> (const Precision<T>&) {
                const auto adapted_field = adapted_reader.cell_field(name);
                expect(eq(adapted_field->layout().dimension(), original_field->layout().dimension()));
                expect(eq(adapted_field->layout().extent(0), original_field->layout().extent(0)*number_of_subdivisions_per_segment));
                if (original_field->layout().dimension() > 1)
                    expect(adapted_field->layout().sub_layout(1) == original_field->layout().sub_layout(1));

                const auto original = original_field->serialized();
                const auto adapted = adapted_field->serialized();
                const std::size_t components = original_field->layout().dimension() > 1 ? original_field->layout().number_of_entries(1) : 1;
                for (std::size_t orig_cell_idx = 0; orig_cell_idx < original_field->layout().extent(0); ++orig_cell_idx) {
                    const auto adapted_cell_idx = orig_cell_idx*number_of_subdivisions_per_segment;
                    for (std::size_t subdivision = 0; subdivision < number_of_subdivisions_per_segment; ++subdivision) {
                        for (std::size_t i = 0; i < components; ++i)
                            expect(eq(
                                original.template as_span_of<T>()[orig_cell_idx*components + i],
                                adapted.template as_span_of<T>()[(adapted_cell_idx + subdivision)*components + i]
                            )) << " (name = " << name
                               << "/ cell idx = " << orig_cell_idx
                               << "/ subdivision = " << subdivision
                               << "/ component = " << i
                               << ")";
                    }
                }
            });
        });
    };
}

int main() {
    test(1);
    test(2);
    return 0;
}
