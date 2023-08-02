// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_TEST_READER_TESTS_HPP_
#define GRIDFORMAT_TEST_READER_TESTS_HPP_

#include <string>
#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/grid/concepts.hpp>

#include "grid/unstructured_grid.hpp"
#include "make_test_data.hpp"
#include "testing.hpp"

namespace GridFormat::Test {

template<typename Factory>
auto make_grid_from_reader(Factory&& factory, GridFormat::GridReader& reader) {
    reader.export_grid(factory);
    return std::move(factory).grid();
}

template<typename Writer, typename Reader>
bool check_equal_fields(const Writer& writer, const Reader& reader, const bool verbose = true) {
    std::vector<std::string> writer_pfields, writer_cfields, writer_mfields;
    for (const auto& [name, _] : point_fields(writer)) writer_pfields.push_back(name);
    for (const auto& [name, _] : cell_fields(writer)) writer_cfields.push_back(name);
    for (const auto& [name, _] : meta_data_fields(writer)) writer_mfields.push_back(name);

    std::vector<std::string> reader_pfields, reader_cfields, reader_mfields;
    std::ranges::copy(point_field_names(reader), std::back_inserter(reader_pfields));
    std::ranges::copy(cell_field_names(reader), std::back_inserter(reader_cfields));
    std::ranges::copy(meta_data_field_names(reader), std::back_inserter(reader_mfields));

    std::ranges::sort(writer_pfields);
    std::ranges::sort(reader_pfields);
    if (!std::ranges::equal(writer_pfields, reader_pfields)) {
        if (verbose) std::cout << "Point fields not equal" << std::endl;
        return false;
    }

    std::ranges::sort(writer_cfields);
    std::ranges::sort(reader_cfields);
    if (!std::ranges::equal(writer_cfields, reader_cfields)) {
        if (verbose) std::cout << "Cell fields not equal" << std::endl;
        return false;
    }

    std::ranges::sort(writer_mfields);
    std::ranges::sort(reader_mfields);
    if (!std::ranges::equal(writer_mfields, reader_mfields)) {
        if (verbose) std::cout << "Metadata fields not equal" << std::endl;
        return false;
    }

    return true;
}

template<std::size_t dim, std::size_t space_dim, typename Writer>
    requires(std::derived_from<Writer, GridWriter<typename Writer::Grid>>)
void test_reader(Writer& writer,
                 GridReader& reader,
                 const std::string& base_filename,
                 const TestFileOptions& opts = {},
                 const bool verbose = true) {
    const auto filename = write_test_file<space_dim>(writer, base_filename, opts, verbose);

    if (verbose)
        std::cout << "Opening '" << as_highlight(filename) << "'" << std::endl;
    reader.open(filename);
    const auto in_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);

    using Testing::operator""_test;
    using Testing::expect;
    using Testing::eq;
    "reader_field_names"_test = [&] () { expect(check_equal_fields(writer, reader)); };
    "reader_grid_num_cells"_test = [&] () {
        expect(eq(number_of_cells(writer.grid()), number_of_cells(in_grid)));
        expect(eq(number_of_cells(writer.grid()), reader.number_of_cells()));
    };
    "reader_grid_num_points"_test = [&] () {
        expect(eq(number_of_points(writer.grid()), number_of_points(in_grid)));
        expect(eq(number_of_points(writer.grid()), reader.number_of_points()));
    };

    writer.clear();
    for (auto [name, fieldptr] : cell_fields(reader))
        writer.set_cell_field(name, fieldptr);
    for (auto [name, fieldptr] : point_fields(reader))
        writer.set_point_field(name, fieldptr);
    for (auto [name, fieldptr] : meta_data_fields(reader))
        writer.set_meta_data(name, fieldptr);
    const auto out_filename = writer.write(base_filename + "_rewritten");

    if (verbose)
        std::cout << "Wrote '" << GridFormat::as_highlight(out_filename) << "'" << std::endl;
}

template<std::size_t dim, std::size_t space_dim, typename Writer, typename WriterFactory>
    requires(std::derived_from<Writer, TimeSeriesGridWriter<typename Writer::Grid>> and
             std::invocable<WriterFactory, const typename Writer::Grid&, const std::string&> and
             std::same_as<Writer, std::invoke_result_t<WriterFactory, const typename Writer::Grid&, const std::string&>>)
void test_reader(Writer& writer,
                 GridReader& reader,
                 const WriterFactory& writer_factory,
                 const TestFileOptions& opts = {},
                 const bool verbose = true) {
    const std::size_t num_steps = 5;
    const auto filename = write_test_time_series<space_dim>(writer, num_steps, opts, verbose);

    if (verbose)
        std::cout << "Opening '" << as_highlight(filename) << "'" << std::endl;
    reader.open(filename);
    using Testing::operator""_test;
    using Testing::expect;
    using Testing::eq;
    "time_series_reader_field_names"_test = [&] () { expect(check_equal_fields(writer, reader)); };
    "time_series_reader_grid_num_steps"_test = [&] () { expect(eq(num_steps, reader.number_of_steps())); };
    "time_series_reader_grid_num_cells"_test = [&] () {
        std::ranges::for_each(std::views::iota(std::size_t{0}, reader.number_of_steps()), [&] (std::size_t step) {
            reader.set_step(step);
            const auto in_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
            expect(eq(GridFormat::number_of_cells(writer.grid()), GridFormat::number_of_cells(in_grid)));
            expect(eq(GridFormat::number_of_cells(writer.grid()), reader.number_of_cells()));
        });
    };
    "time_series_reader_grid_num_points"_test = [&] () {
        std::ranges::for_each(std::views::iota(std::size_t{0}, reader.number_of_steps()), [&] (std::size_t step) {
            reader.set_step(step);
            const auto in_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
            expect(eq(GridFormat::number_of_points(writer.grid()), GridFormat::number_of_points(in_grid)));
            expect(eq(GridFormat::number_of_points(writer.grid()), reader.number_of_points()));
        });
    };

    std::string out_filename;
    auto out_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
    auto out_writer = writer_factory(out_grid, filename + "_rewritten");
    for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
        reader.set_step(step);
        out_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
        for (auto [name, fieldptr] : cell_fields(reader))
            out_writer.set_cell_field(name, fieldptr);
        for (auto [name, fieldptr] : point_fields(reader))
            out_writer.set_point_field(name, fieldptr);
        for (auto [name, fieldptr] : meta_data_fields(reader))
            out_writer.set_meta_data(name, fieldptr);
        out_filename = out_writer.write(reader.time_at_step(step));
    }

    if (verbose)
        std::cout << "Wrote '" << GridFormat::as_highlight(out_filename) << "'" << std::endl;
}

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_READER_TESTS_HPP_
