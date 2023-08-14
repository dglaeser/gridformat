// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <optional>
#include <algorithm>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <ranges>
#include <cmath>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/gridformat.hpp>

#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/pvd_writer.hpp>

#include <gridformat/vtk/vts_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

#ifndef TEST_VTK_DATA_PATH
#define TEST_VTK_DATA_PATH ""
#endif

#ifndef RUN_PARALLEL
#define RUN_PARALLEL 0
#endif

#if RUN_PARALLEL
#include <mpi.h>
#endif

bool is_scalar_field(GridFormat::FieldPtr field) {
    const auto layout = field->layout();
    return layout.dimension() == 1 || (layout.dimension() > 1 && layout.number_of_entries(1) == 1);
}

bool eq(const auto& a, const auto& b) {
    return a == b;
}

void expect(const auto& expr, std::source_location s = std::source_location::current()) {
    if (!static_cast<bool>(expr))
        throw GridFormat::ValueError("Unexpected value", s);
}

auto grid_and_space_dimension(const std::string& filename) {
    const auto pos = filename.find("d_in_");
    if (pos == std::string::npos || pos == 0 || pos > filename.size() - 7)
        throw GridFormat::ValueError("Could not deduce grid & space dim from filename '" + filename + "'");
    const unsigned int grid_dim = std::stoi(filename.substr(pos-1, 1));
    const unsigned int space_dim = std::stoi(filename.substr(pos+5, 1));
    return std::make_pair(grid_dim, space_dim);
}

std::ranges::range auto test_filenames(const std::filesystem::path& folder, const std::string& extension) {
    std::vector<std::string> names;
    if (!std::filesystem::directory_entry{folder}.exists())
        return names;
    std::ranges::copy(
        std::filesystem::directory_iterator{folder}
        | std::views::filter([&] (const auto& p) { return std::filesystem::is_regular_file(p); })
        | std::views::filter([&] (const std::filesystem::path& p) { return p.extension() == extension; })
        | std::views::transform([&] (const std::filesystem::path& p) { return p.string(); }),
        std::back_inserter(names)
    );
    return names;
}

void test_reader(GridFormat::Reader&& reader, const std::string& filename) {
    std::cout << "Testing reader with '" << GridFormat::as_highlight(filename) << "'" << std::endl;

    reader.open(filename);
    const auto points = reader.points()->template export_to<std::vector<std::array<double, 3>>>();
    const auto [_, space_dim] = grid_and_space_dimension(filename);
    const auto get_expected_value = [&] (const std::array<double, 3>& position, double t = 1.0) -> double {
        if (space_dim == 1)
            return GridFormat::Test::test_function<double>(std::array{position[0]}, t);
        if (space_dim == 2)
            return GridFormat::Test::test_function<double>(std::array{position[0], position[1]}, t);
        return GridFormat::Test::test_function<double>(position, t);
    };

    std::optional<std::size_t> num_steps;
    if (reader.is_sequence())
        num_steps = reader.number_of_steps();

    const std::size_t num_cell_fields = GridFormat::Ranges::size(cell_field_names(reader));
    const std::size_t num_point_fields = GridFormat::Ranges::size(point_field_names(reader));
    const std::size_t num_meta_data_fields = GridFormat::Ranges::size(meta_data_field_names(reader));

    for (std::size_t step = 0; step < num_steps.value_or(std::size_t{1}); ++step) {
        double step_time = 1.0;
        if (num_steps) {
            std::cout << "Setting step " << step << std::endl;
            reader.set_step(step);
            step_time = reader.time_at_step(step);
        }

        for (const auto [name, fieldptr] : point_fields(reader)) {
            if (is_scalar_field(fieldptr)) {
                const auto values = fieldptr->template export_to<std::vector<double>>();
                std::size_t p_idx = 0;
                bool all_equal = true;
                std::ranges::for_each(values, [&] (const auto& value) {
                    const auto& point = points[p_idx];
                    const auto expected_value = get_expected_value(point, step_time);
                    all_equal = all_equal && GridFormat::Test::equals(expected_value, value);
                    p_idx++;
                });
                expect(eq(reader.number_of_points(), p_idx));
                expect(all_equal);
            }
        }

        for (const auto [name, fieldptr] : cell_fields(reader)) {
            if (is_scalar_field(fieldptr)) {
                const auto values = fieldptr->template export_to<std::vector<double>>();
                std::size_t c_idx = 0;
                bool all_equal = true;
                reader.visit_cells([&] (GridFormat::CellType, const std::vector<std::size_t>& corners) {
                    std::array<double, 3> center{0., 0., 0.};
                    std::ranges::for_each(corners, [&] (const auto corner_idx) {
                        const auto& corner = points.at(corner_idx);
                        center[0] += corner[0];
                        center[1] += corner[1];
                        center[2] += corner[2];
                    });
                    std::ranges::for_each(center, [&] (auto& c) { c /= static_cast<double>(corners.size()); });
                    const auto expected_value = get_expected_value(center, step_time);
                    all_equal = all_equal && GridFormat::Test::equals(expected_value, values[c_idx]);
                    c_idx++;
                });
                expect(eq(reader.number_of_cells(), c_idx));
                expect(all_equal);
            }
        }

        expect(eq(num_cell_fields, GridFormat::Ranges::size(point_field_names(reader))));
        expect(eq(num_point_fields, GridFormat::Ranges::size(cell_field_names(reader))));
        expect(eq(num_meta_data_fields, GridFormat::Ranges::size(meta_data_field_names(reader))));
        std::cout << "Visited "
                  << num_point_fields << " / "
                  << num_cell_fields << " / "
                  << num_meta_data_fields
                  << " point / cell / meta data fields"
                  << std::endl;
    }
}

template<typename... ConstructorArgs>
int test_reader(const std::filesystem::path& folder,
                const std::string& extension,
                ConstructorArgs&&... args) {
    bool visited = false;
    std::ranges::for_each(test_filenames(folder, extension), [&] (const std::string& filename) {
        test_reader(GridFormat::Reader{std::forward<ConstructorArgs>(args)...}, filename);
        visited = true;

        if (extension == ".vtu") { // exemplarily test that reader propagates names
            GridFormat::Reader reader{GridFormat::vtu};
            reader.open(filename);
            GridFormat::Testing::expect(reader.name() == "VTUReader");
        }
    });
    if (!visited)
        std::cout << "Could not find test data files for extension "
                  << extension << " in folder " << std::string{folder}
                  << std::endl;
    return visited ? 0 : 42;
}

template<std::size_t dim, typename Writer, typename... Args>
std::string write_test_file(Writer&& w, Args&&... args) {
    return GridFormat::Test::write_test_file<dim>(w, std::forward<Args>(args)...);
}

template<std::size_t dim, typename Writer, typename... Args>
std::string write_test_time_series(Writer&& w, Args&&... args) {
    return GridFormat::Test::write_test_time_series<dim>(w, std::forward<Args>(args)...);
}

template<typename Format, typename Grid, typename Communicator>
auto make_writer(const Format& format, const Grid& grid, const Communicator& comm) {
    using Factory = GridFormat::WriterFactory<Format>;
    if constexpr (std::is_same_v<Communicator, GridFormat::NullCommunicator>)
        return Factory::make(format, grid);
    else
        return Factory::make(format, grid, comm);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
#if RUN_PARALLEL
    MPI_Init(&argc, &argv);
    const auto comm = MPI_COMM_WORLD;
    constexpr bool is_parallel = true;
#else
    const GridFormat::NullCommunicator comm{};
    constexpr bool is_parallel = false;
#endif

    std::vector<int> exit_codes;
    const std::string vtk_test_data_folder{TEST_VTK_DATA_PATH};

    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtu", GridFormat::vtu));
    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtu", GridFormat::any));

    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtp", GridFormat::vtp));
    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtp", GridFormat::any));

    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vti", GridFormat::vti));
    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vti", GridFormat::any));

    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtr", GridFormat::vtr));
    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vtr", GridFormat::any));

    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vts", GridFormat::vts));
    exit_codes.push_back(test_reader(vtk_test_data_folder, ".vts", GridFormat::any));

    // generate some more test data & test it
    const auto comm_size = GridFormat::Parallel::size(comm);
    const auto comm_rank = GridFormat::Parallel::rank(comm);
    const double x_offset = comm_rank%2;
    const double y_offset = comm_rank/2;
    if (comm_size != 1 && comm_size%2 != 0)
        throw GridFormat::ValueError("Communicator size must be 1 or divisible by 2");

    const GridFormat::Test::StructuredGrid<2> grid{{1.0, 1.0}, {4, 5}, {x_offset, y_offset}};
    const std::string parallel_suffix = is_parallel ? "_parallel" : "";
    const std::filesystem::path generated_data_folder{"generated_test_data" + parallel_suffix};
    std::filesystem::create_directory(generated_data_folder);

    std::vector<std::string> test_filenames;
    const auto make_filename = [&] (const std::string& keyword) {
        return "generic_reader_" + keyword + "_2d_in_2d" + parallel_suffix;
    };
    const auto add_test_file = [&] (std::filesystem::path path) {
        if (path.parent_path() != generated_data_folder)
            throw GridFormat::IOError("Unexpected generated test data path: " + std::string{path});
        test_filenames.push_back(path.filename());
    };

    add_test_file(write_test_time_series<2>(GridFormat::PVDWriter{
        make_writer(GridFormat::vtu, grid, comm), generated_data_folder / make_filename("vtu")
    }));
    add_test_file(write_test_time_series<2>(GridFormat::PVDWriter{
        make_writer(GridFormat::vtp, grid, comm), generated_data_folder / make_filename("vtp")
    }));
    add_test_file(write_test_time_series<2>(GridFormat::PVDWriter{
        make_writer(GridFormat::vti, grid, comm), generated_data_folder / make_filename("vti")
    }));
    const auto vti_filename = test_filenames.back();

    add_test_file(write_test_time_series<2>(GridFormat::PVDWriter{
        make_writer(GridFormat::vtr, grid, comm), generated_data_folder / make_filename("vtr")
    }));
    const auto vtr_filename = test_filenames.back();

    add_test_file(write_test_time_series<2>(GridFormat::PVDWriter{
        make_writer(GridFormat::vts, grid, comm), generated_data_folder / make_filename("vts")
    }));
    if constexpr (is_parallel) {
        test_reader(generated_data_folder, ".pvd", GridFormat::pvd, comm);
        test_reader(generated_data_folder, ".pvd", GridFormat::any, comm);
    } else {
        test_reader(generated_data_folder, ".pvd", GridFormat::pvd);
        test_reader(generated_data_folder, ".pvd", GridFormat::any);
    }

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "generic_reader_throws_on_non_matching_format"_test = [&] () {
        GridFormat::Reader r{GridFormat::pvd_with(GridFormat::vtr)};
        r.open(generated_data_folder / vtr_filename);
        GridFormat::Testing::expect(throws<GridFormat::IOError>([&] () { r.open(vti_filename); }));
    };

    // check that reader exposes image/rectilinear grid-specific interfaces
    if (!is_parallel) {
        const auto _vti_test = [&] (auto& vti_reader) {
            vti_reader.open(generated_data_folder / vti_filename);

            expect(eq(vti_reader.extents()[0], std::size_t{4}));
            expect(eq(vti_reader.extents()[1], std::size_t{5}));
            expect(eq(vti_reader.extents()[2], std::size_t{0}));

            expect(eq(vti_reader.location().lower_left[0], std::size_t{0}));
            expect(eq(vti_reader.location().lower_left[1], std::size_t{0}));
            expect(eq(vti_reader.location().lower_left[2], std::size_t{0}));

            expect(eq(vti_reader.location().upper_right[0], std::size_t{4}));
            expect(eq(vti_reader.location().upper_right[1], std::size_t{5}));
            expect(eq(vti_reader.location().upper_right[2], std::size_t{0}));

            expect(eq(vti_reader.origin()[0], 0.0));
            expect(eq(vti_reader.origin()[1], 0.0));
            expect(eq(vti_reader.origin()[2], 0.0));

            expect(std::abs(vti_reader.spacing()[0] - 1.0/4.0) < 1e-6);
            expect(std::abs(vti_reader.spacing()[1] - 1.0/5.0) < 1e-6);
            expect(std::abs(vti_reader.spacing()[2] - 0.0) < 1e-6);
        };

        "generic_reader_vti_interfaces_unbound"_test = [&] () {
            GridFormat::Reader generic_reader;
            _vti_test(generic_reader);
        };

        "generic_reader_vti_interfaces_bound"_test = [&] () {
            GridFormat::Reader bound_reader{GridFormat::pvd_with(GridFormat::vti)};
            _vti_test(bound_reader);
        };

        const auto _vtr_test = [&] (auto& vtr_reader) {
            vtr_reader.open(generated_data_folder / vtr_filename);
            for (unsigned int dir = 0; dir < 3; ++dir) {
                const double spacing = std::array{1.0/4.0, 1.0/5.0, 0.0}[dir];
                expect(std::ranges::all_of(vtr_reader.ordinates(dir), [&, i=0] (const auto& x) mutable {
                    return std::abs(x - spacing*static_cast<double>(i++)) < 1e-6;
                }));
            }
        };

        "generic_reader_vtr_interfaces_unbound"_test = [&] () {
            GridFormat::Reader generic_reader;
            _vtr_test(generic_reader);
        };

        "generic_reader_vtr_interfaces_bound"_test = [&] () {
            GridFormat::Reader bound_reader{GridFormat::pvd_with(GridFormat::vtr)};
            _vtr_test(bound_reader);
        };
    }

#if GRIDFORMAT_HAVE_HIGH_FIVE
    add_test_file(write_test_file<2>(
        GridFormat::VTKHDFUnstructuredGridWriter{grid, comm},
        generated_data_folder / make_filename("hdf_unstructured")
    ));
    add_test_file(write_test_file<2>(
        GridFormat::VTKHDFImageGridWriter{grid, comm},
        generated_data_folder / make_filename("hdf_image")
    ));
    add_test_file(write_test_time_series<2>(
        GridFormat::VTKHDFImageGridTimeSeriesWriter{
            grid, comm, generated_data_folder / make_filename("hdf_image_ts")
        }
    ));
    add_test_file(write_test_time_series<2>(
        GridFormat::VTKHDFUnstructuredTimeSeriesWriter{
            grid, comm, generated_data_folder / make_filename("hdf_unstructured_ts")
        }
    ));

    test_reader(generated_data_folder, ".hdf", GridFormat::vtk_hdf, comm);
    test_reader(generated_data_folder, ".hdf", GridFormat::any, comm);
#endif  // GRIDFORMAT_HAVE_HIGH_FIVE

#if RUN_PARALLEL
    MPI_Finalize();
#endif

    return *std::max_element(exit_codes.begin(), exit_codes.end());
}
