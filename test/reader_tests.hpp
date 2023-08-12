// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_TEST_READER_TESTS_HPP_
#define GRIDFORMAT_TEST_READER_TESTS_HPP_

#include <string>
#include <utility>
#include <concepts>
#include <type_traits>
#include <limits>
#include <cmath>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/ranges.hpp>

#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/grid/concepts.hpp>

#include "grid/unstructured_grid.hpp"
#include "make_test_data.hpp"
#include "testing.hpp"

namespace GridFormat::Test {

template<typename T1, typename T2, typename T3 = double, typename T4 = double>
bool equals(const T1& _a,
            const T2& _b,
            const T3& _rel_tol = 1e-5,
            const T4& _abs_tol = std::numeric_limits<T4>::epsilon()) {
    using CommonType = std::common_type_t<T1, T2, T3, T4>;
    const auto a = static_cast<CommonType>(_a);
    const auto b = static_cast<CommonType>(_b);
    const auto rel_tol = static_cast<CommonType>(_rel_tol);
    const auto abs_tol = static_cast<CommonType>(_abs_tol);
    using std::abs;
    using std::max;
    return abs(a-b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol);
}

template<typename Factory>
auto make_grid_from_reader(Factory&& factory, GridFormat::GridReader& reader) {
    reader.export_grid(factory);
    return std::move(factory).grid();
}

template<unsigned int orig_space_dim,
         unsigned int vector_space_dim = 3,
         typename Grid,
         std::ranges::range EntityRange>
bool test_field_values(std::string_view name,
                       GridFormat::FieldPtr field_ptr,
                       const Grid& grid,
                       const EntityRange& entities,
                       double time_at_step = 1.0,
                       const int verbose = 1) {
    if (verbose > 1)
        std::cout << "Testing field '" << name << "' (at t = " << time_at_step << ")" << std::endl;

    const auto layout = field_ptr->layout();
    if (layout.extent(0) != Ranges::size(entities)) {
        std::cout << "Size mismatch between field and number of entities" << std::endl;
        return false;
    }

    return field_ptr->precision().visit([&] <typename T> (const Precision<T>& precision) {
        if constexpr (std::floating_point<T>) {
            std::size_t i = 0;
            auto field_data = field_ptr->serialized();
            auto field_values = field_data.as_span_of(precision);
            const auto ncomps = layout.dimension() > 1 ? layout.number_of_entries(1) : 1;
            for (const auto& e : entities) {
                const auto eval_pos = evaluation_position(grid, e);
                const auto test_value = test_function<T>(eval_pos, time_at_step);
                for (unsigned int comp = 0; comp < ncomps; ++comp) {
                    const bool use_zero = comp/vector_space_dim >= orig_space_dim
                                            || comp%vector_space_dim >= orig_space_dim;
                    const T expected_value = use_zero ? 0 : test_value;
                    if (field_values.size() < i)
                        throw SizeError("Field values too short");
                    if (!equals(field_values[i], expected_value)) {
                        std::cout << "Found deviation for field " << name
                                  << " (at time step t = " << time_at_step << "), "
                                  << "at " << as_string(eval_pos) << ": "
                                  << field_values[i] << " - " << expected_value << " "
                                  << "(comp = " << comp << "; vector_space_dim = " << vector_space_dim << ")"
                                  << std::endl;
                        return false;
                    }
                    i++;
                }
            }
        } else {
            log_warning("Unsupported field value type, skipping test...");
        }

        return true;
    });
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

template<std::size_t dim,
         std::size_t space_dim,
         std::size_t vector_space_dim = 3,
         typename Writer>
    requires(std::derived_from<Writer, GridWriter<typename Writer::Grid>>)
std::string test_reader(Writer& writer,
                        GridReader& reader,
                        const std::string& base_filename,
                        const TestFileOptions& opts = {},
                        const int verbose = 1) {
    const auto filename = write_test_file<space_dim>(writer, base_filename, opts, verbose);

    if (verbose > 0)
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

    // test field values and also write out a new file which can be regression-tested
    "reader_field_values"_test = [&] () {
        writer.clear();
        for (auto [name, fieldptr] : cell_fields(reader)) {
            writer.set_cell_field(name, fieldptr);
            expect(test_field_values<space_dim, vector_space_dim>(name, fieldptr, in_grid, cells(in_grid), 1.0, verbose));
        }
        for (auto [name, fieldptr] : point_fields(reader)) {
            writer.set_point_field(name, fieldptr);
            expect(test_field_values<space_dim, vector_space_dim>(name, fieldptr, in_grid, points(in_grid), 1.0, verbose));
        }
        for (auto [name, fieldptr] : meta_data_fields(reader))
            writer.set_meta_data(name, fieldptr);
        const auto out_filename = writer.write(base_filename + "_rewritten");
        if (verbose > 0)
            std::cout << "Wrote '" << GridFormat::as_highlight(out_filename) << "'" << std::endl;
    };

    return filename;
}

template<std::size_t dim,
         std::size_t space_dim,
         std::size_t vector_space_dim = 3,
         typename Writer,
         typename WriterFactory>
    requires(std::derived_from<Writer, TimeSeriesGridWriter<typename Writer::Grid>> and
             std::invocable<WriterFactory, const typename Writer::Grid&, const std::string&> and
             std::derived_from<
                std::invoke_result_t<WriterFactory, const typename Writer::Grid&, const std::string&>,
                TimeSeriesGridWriter<typename Writer::Grid>
            >)
std::string test_reader(Writer& writer,
                        GridReader& reader,
                        const WriterFactory& writer_factory,
                        const TestFileOptions& opts = {},
                        const int verbose = 1) {
    const std::size_t num_steps = 5;
    const auto filename = write_test_time_series<space_dim>(writer, num_steps, opts, verbose);

    if (verbose > 0)
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

    // test field values and also write out a new file which can be regression-tested
    "reader_field_values"_test = [&] () {
        std::string out_filename;
        auto out_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
        auto out_writer = writer_factory(out_grid, filename + "_rewritten");
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            const auto time_at_step = reader.time_at_step(step);
            if (verbose > 2)
                std::cout << "Testing field values at time = " << time_at_step << std::endl;

            reader.set_step(step);
            expect(check_equal_fields(writer, reader));
            out_grid = make_grid_from_reader(UnstructuredGridFactory<dim, space_dim>{}, reader);
            for (auto [name, fieldptr] : cell_fields(reader)) {
                out_writer.set_cell_field(name, fieldptr);
                expect(test_field_values<space_dim, vector_space_dim>(
                    name, fieldptr, out_grid, cells(out_grid), time_at_step, verbose
                ));
            }
            for (auto [name, fieldptr] : point_fields(reader)) {
                out_writer.set_point_field(name, fieldptr);
                expect(test_field_values<space_dim, vector_space_dim>(
                    name, fieldptr, out_grid, points(out_grid), time_at_step, verbose
                ));
            }
            for (auto [name, fieldptr] : meta_data_fields(reader))
                out_writer.set_meta_data(name, fieldptr);
            out_filename = out_writer.write(time_at_step);
        }

        if (verbose > 0)
            std::cout << "Wrote '" << GridFormat::as_highlight(out_filename) << "'" << std::endl;
    };

    return filename;
}

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_READER_TESTS_HPP_
