// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <vector>
#include <array>
#include <cstddef>
#include <filesystem>

#include <gridformat/common/hdf5.hpp>
#include <gridformat/common/multi_string.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../testing.hpp"

using GridFormat::Testing::operator""_test;
using GridFormat::Testing::expect;
using GridFormat::Testing::throws;
using GridFormat::Testing::eq;

namespace {

constexpr auto literal_text = "some_literal_text";
const std::string string_text{"some_string_text"};

template<typename Writer>
void set_meta_data(Writer& writer) {
    writer.set_meta_data("literal", literal_text);
    writer.set_meta_data("string", string_text);
    writer.set_meta_data("numbers", std::vector<int>{1, 2, 3, 4});
}

HighFive::DataSet open_field_data(HighFive::File& file, const std::string& name) {
    return file.getDataSet("/VTKHDF/FieldData/" + name);
}

//! Check that the given dataset is a (1,)-shaped dataset of variable-length strings
void expect_string_dataset(HighFive::File& file, const std::string& name, const std::string& expected) {
    auto dataset = open_field_data(file, name);
    const auto type = dataset.getDataType();
    expect(type.getClass() == HighFive::DataTypeClass::String);
    expect(type.isVariableStr());

    const auto dims = dataset.getDimensions();
    expect(eq(dims.size(), std::size_t{1}));
    expect(eq(dims.at(0), std::size_t{1}));

    std::vector<std::string> values;
    dataset.read(values);
    expect(eq(values.size(), std::size_t{1}));
    expect(eq(values.at(0), expected));
}

//! Numeric meta data must be unaffected by the string format
void expect_unchanged_numbers(HighFive::File& file) {
    auto dataset = open_field_data(file, "numbers");
    expect(dataset.getDataType().getClass() == HighFive::DataTypeClass::Integer);
    expect(eq(dataset.getDataType().getSize(), std::size_t{4}));
    expect(eq(dataset.getDimensions().size(), std::size_t{1}));
    expect(eq(dataset.getDimensions().at(0), std::size_t{4}));
    expect(std::ranges::equal(dataset.read<std::vector<int>>(), std::vector<int>{1, 2, 3, 4}));
}

}  // namespace

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();

    "vtk_hdf_string_meta_data_as_string_array"_test = [&] () {
        GridFormat::VTKHDFWriter writer{grid};
        set_meta_data(writer);
        const auto filename = writer.write("vtk_hdf_string_meta_data_string_array");

        HighFive::File file{filename, HighFive::File::ReadOnly};
        expect_string_dataset(file, "literal", literal_text);
        expect_string_dataset(file, "string", string_text);
        expect_unchanged_numbers(file);
    };

    "vtk_hdf_string_meta_data_read_back"_test = [&] () {
        GridFormat::VTKHDFReader reader;
        reader.open("vtk_hdf_string_meta_data_string_array.hdf");
        expect(eq(reader.meta_data_field("literal")->template export_to<std::string>(), std::string{literal_text}));
        expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), string_text));
        expect(std::ranges::equal(
            reader.meta_data_field("numbers")->template export_to<std::vector<int>>(),
            std::vector<int>{1, 2, 3, 4}
        ));
    };

    // the value has to differ per step, otherwise reading the wrong entry goes unnoticed
    const auto text_at = [] (std::size_t step) { return "step_" + std::to_string(step); };
    const auto write_transient = [&] (const std::string& base, bool static_meta_data) {
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid, base, GridFormat::VTK::HDFTransientOptions{
                .static_grid = true,
                .static_meta_data = static_meta_data
            }
        };
        std::size_t step = 0;
        for (double t : {0.0, 0.5, 1.0}) {
            writer.set_meta_data("string", text_at(step++));
            writer.write(t);
        }
    };

    "vtk_hdf_transient_string_meta_data"_test = [&] () {
        const std::string base = "vtk_hdf_string_meta_data_transient";
        write_transient(base, false);

        // one variable-length string per step, selected via Steps/FieldDataOffsets
        HighFive::File file{base + ".hdf", HighFive::File::ReadOnly};
        auto dataset = open_field_data(file, "string");
        expect(dataset.getDataType().isVariableStr());
        expect(eq(dataset.getDimensions().size(), std::size_t{1}));
        expect(eq(dataset.getDimensions().at(0), std::size_t{3}));
        expect(std::ranges::equal(
            dataset.read<std::vector<std::string>>(),
            std::vector<std::string>{text_at(0), text_at(1), text_at(2)}
        ));
        expect(std::ranges::equal(
            file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/string").read<std::vector<std::size_t>>(),
            std::vector<std::size_t>{0, 1, 2}
        ));

        GridFormat::VTKHDFReader reader;
        reader.open(base + ".hdf");
        expect(eq(reader.number_of_steps(), std::size_t{3}));
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), text_at(step)));
        }
    };

    "vtk_hdf_static_transient_string_meta_data"_test = [&] () {
        const std::string base = "vtk_hdf_string_meta_data_transient_static";
        write_transient(base, true);

        // written once, with all steps pointing at the first entry
        HighFive::File file{base + ".hdf", HighFive::File::ReadOnly};
        auto dataset = open_field_data(file, "string");
        expect(dataset.getDataType().isVariableStr());
        expect(eq(dataset.getDimensions().at(0), std::size_t{1}));
        expect(std::ranges::equal(
            file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/string").read<std::vector<std::size_t>>(),
            std::vector<std::size_t>{0, 0, 0}
        ));

        GridFormat::VTKHDFReader reader;
        reader.open(base + ".hdf");
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            expect(eq(reader.meta_data_field("string")->template export_to<std::string>(), text_at(0)));
        }
    };

    "vtk_hdf_multi_string_meta_data"_test = [&] () {
        // VTK writes a vtkStringArray with N values as a dataset with N variable-length strings
        const std::vector<std::string> values{"alpha", "", "gamma"};
        GridFormat::VTKHDFWriter writer{grid};
        writer.set_meta_data("names", GridFormat::MultiString{values});
        const auto filename = writer.write("vtk_hdf_string_meta_data_multi");
        {
            HighFive::File file{filename, HighFive::File::ReadOnly};
            auto dataset = open_field_data(file, "names");
            expect(dataset.getDataType().isVariableStr());
            expect(std::ranges::equal(dataset.read<std::vector<std::string>>(), values));
        }

        GridFormat::VTKHDFReader reader;
        reader.open(filename);
        const auto field = reader.meta_data_field("names");
        const auto strings = field->template export_to<GridFormat::MultiString>();
        expect(std::ranges::equal(strings.slices(), values));
        expect(throws([&] () { field->template export_to<std::string>(); }));
    };

    // the number of strings differs per step, which requires the offsets and sizes of each step
    const std::vector<std::vector<std::string>> strings_at{{"a"}, {"b", "c"}, {"", "d", "e"}};
    const auto write_transient_multi = [&] (const std::string& base, bool static_meta_data) {
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid, base, GridFormat::VTK::HDFTransientOptions{
                .static_grid = true,
                .static_meta_data = static_meta_data
            }
        };
        for (std::size_t step = 0; step < strings_at.size(); ++step) {
            writer.set_meta_data("names", GridFormat::MultiString{strings_at[step]});
            writer.write(static_cast<double>(step));
        }
        return base + ".hdf";
    };
    const auto read_names = [&] (const std::string& filename) {
        std::vector<std::vector<std::string>> result;
        GridFormat::VTKHDFReader reader;
        reader.open(filename);
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            const auto strings = reader.meta_data_field("names")->template export_to<GridFormat::MultiString>();
            const auto slices = strings.slices();
            result.emplace_back(slices.begin(), slices.end());
        }
        return result;
    };
    const auto read_sizes = [&] (HighFive::File& file, const std::string& name = "names") {
        return file.getDataSet("/VTKHDF/Steps/FieldDataSizes/" + name).read<std::vector<std::vector<std::size_t>>>();
    };
    const auto read_offsets = [&] (HighFive::File& file, const std::string& name) {
        return file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/" + name).read<std::vector<std::size_t>>();
    };

    "vtk_hdf_transient_multi_string_meta_data"_test = [&] () {
        const auto filename = write_transient_multi("vtk_hdf_string_meta_data_transient_multi", false);
        {   // same layout as written by VTK: concatenated strings, with offsets and (components, tuples) per step
            HighFive::File file{filename, HighFive::File::ReadOnly};
            expect(eq(open_field_data(file, "names").getDimensions().at(0), std::size_t{6}));
            expect(std::ranges::equal(
                file.getDataSet("/VTKHDF/Steps/FieldDataOffsets/names").read<std::vector<std::size_t>>(),
                std::vector<std::size_t>{0, 1, 3}
            ));
            expect(read_sizes(file) == std::vector<std::vector<std::size_t>>{{1, 1}, {1, 2}, {1, 3}});
        }
        expect(read_names(filename) == strings_at);
    };

    "vtk_hdf_transient_multi_component_strings"_test = [&] () {
        // other writers may store strings with multiple components per tuple
        const auto filename = write_transient_multi("vtk_hdf_string_meta_data_transient_multi_component", false);
        {
            HighFive::File file{filename, HighFive::File::ReadWrite};
            file.getDataSet("/VTKHDF/Steps/FieldDataSizes/names").write(
                std::vector<std::vector<std::size_t>>{{1, 1}, {2, 1}, {1, 3}}
            );
        }
        expect(read_names(filename) == strings_at);
    };

    "vtk_hdf_static_transient_multi_string_meta_data"_test = [&] () {
        const auto filename = write_transient_multi("vtk_hdf_string_meta_data_static_transient_multi", true);
        {   // written once, with all steps pointing at the strings of the first step
            HighFive::File file{filename, HighFive::File::ReadOnly};
            expect(eq(open_field_data(file, "names").getDimensions().at(0), std::size_t{1}));
            expect(read_sizes(file) == std::vector<std::vector<std::size_t>>{{1, 1}, {1, 1}, {1, 1}});
        }
        expect(read_names(filename) == std::vector<std::vector<std::string>>(3, strings_at.front()));
    };

    // numeric field data with a different number of tuples per step, and with multiple components
    using Vector = std::array<double, 3>;
    const auto vectors_at = [] (std::size_t step) {
        return std::vector<Vector>(step + 1, Vector{static_cast<double>(step), 1.0, 2.0});
    };
    const auto write_transient_numbers = [&] (const std::string& base, bool static_meta_data) {
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid, base, GridFormat::VTK::HDFTransientOptions{
                .static_grid = true,
                .static_meta_data = static_meta_data
            }
        };
        for (std::size_t step = 0; step < 3; ++step) {
            writer.set_meta_data("vectors", vectors_at(step));
            writer.set_meta_data("scalar", static_cast<int>(step));
            writer.write(static_cast<double>(step));
        }
        return base + ".hdf";
    };
    const auto expect_numbers = [&] (const std::string& filename, auto&& step_of) {
        GridFormat::VTKHDFReader reader;
        reader.open(filename);
        expect(eq(reader.number_of_steps(), std::size_t{3}));
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            const auto vectors = reader.meta_data_field("vectors");
            expect(eq(vectors->layout().extent(0), step_of(step) + 1));
            expect(vectors->template export_to<std::vector<Vector>>() == vectors_at(step_of(step)));
            expect(eq(reader.meta_data_field("scalar")->template export_to<int>(), static_cast<int>(step_of(step))));
        }
    };

    "vtk_hdf_transient_numeric_field_data"_test = [&] () {
        const auto filename = write_transient_numbers("vtk_hdf_field_data_transient_numbers", false);
        {   // same layout as written by VTK: concatenated tuples, with offsets and (components, tuples) per step
            HighFive::File file{filename, HighFive::File::ReadOnly};
            expect(open_field_data(file, "vectors").getDimensions() == std::vector<std::size_t>{6, 3});
            expect(read_offsets(file, "vectors") == std::vector<std::size_t>{0, 1, 3});
            expect(read_sizes(file, "vectors") == std::vector<std::vector<std::size_t>>{{3, 1}, {3, 2}, {3, 3}});
            expect(open_field_data(file, "scalar").getDimensions() == std::vector<std::size_t>{3});
            expect(read_offsets(file, "scalar") == std::vector<std::size_t>{0, 1, 2});
            expect(read_sizes(file, "scalar") == std::vector<std::vector<std::size_t>>{{1, 1}, {1, 1}, {1, 1}});
        }
        expect_numbers(filename, [] (std::size_t step) { return step; });
    };

    "vtk_hdf_static_transient_numeric_field_data"_test = [&] () {
        const auto filename = write_transient_numbers("vtk_hdf_field_data_static_transient_numbers", true);
        {
            HighFive::File file{filename, HighFive::File::ReadOnly};
            expect(open_field_data(file, "vectors").getDimensions() == std::vector<std::size_t>{1, 3});
            expect(read_offsets(file, "vectors") == std::vector<std::size_t>{0, 0, 0});
            expect(read_sizes(file, "vectors") == std::vector<std::vector<std::size_t>>{{3, 1}, {3, 1}, {3, 1}});
        }
        expect_numbers(filename, [] (std::size_t) { return std::size_t{0}; });
    };

    "hdf5_append_rejects_mismatching_types"_test = [&] () {
        using File = GridFormat::HDF5::File<>;
        const std::string filename = "vtk_hdf_string_meta_data_type_mismatch.hdf";
        File::clear(filename, GridFormat::NullCommunicator{});
        {   // create both in append mode, such that they are extendible
            File file{filename, File::append};
            file.write(std::vector<int>{1, 2, 3}, "/numeric");
            file.write_strings({"a"}, "/strings");
        }
        {
            File file{filename, File::append};
            expect(throws([&] () { file.write_strings({"b"}, "/numeric"); }));
            expect(throws([&] () { file.write(std::vector<int>{4}, "/strings"); }));
            expect(throws([&] () { file.write(std::vector<double>{4.0}, "/numeric"); }));
            expect(throws([&] () { file.write(std::vector<long>{4}, "/numeric"); }));
            file.write(std::vector<int>{4}, "/numeric");
            // the encoding of variable-length strings may differ between appends
            file.write_strings({"\xc3\xa4"}, "/strings");
        }
        {   // the rejected writes must have left the datasets untouched
            File file{filename, File::read_only};
            expect(eq(file.get_dimensions("/numeric").value().at(0), std::size_t{4}));
            expect(eq(file.get_dimensions("/strings").value().at(0), std::size_t{2}));
            expect(std::ranges::equal(
                file.read_dataset_to<std::vector<int>>("/numeric"), std::vector<int>{1, 2, 3, 4}
            ));
        }
        {   // datasets created in append mode must be extendible without limit
            HighFive::File file{filename, HighFive::File::ReadOnly};
            const auto max_dims = file.getDataSet("/numeric").getSpace().getMaxDimensions();
            expect(max_dims.at(0) == HighFive::DataSpace::UNLIMITED);
        }
    };

    "vtk_hdf_invalid_array_names"_test = [&] () {
        const auto write_with_meta_data_name = [&] (const std::string& name) {
            GridFormat::VTKHDFWriter writer{grid};
            writer.set_meta_data(name, string_text);
            writer.write("vtk_hdf_string_meta_data_invalid_name");
        };
        expect(throws([&] () { write_with_meta_data_name("with/slash"); }));
        expect(throws([&] () { write_with_meta_data_name("with.dot"); }));

        const auto write_with_cell_field_name = [&] (const std::string& name) {
            GridFormat::VTKHDFWriter writer{grid};
            writer.set_cell_field(name, [] (const auto&) { return 1.0; });
            writer.write("vtk_hdf_string_meta_data_invalid_name");
        };
        expect(throws([&] () { write_with_cell_field_name("with/slash"); }));
        expect(throws([&] () { write_with_cell_field_name("with.dot"); }));
    };

    return 0;
}
