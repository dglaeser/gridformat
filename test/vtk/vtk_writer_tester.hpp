// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_TEST_VTK_WRITER_TESTER_HPP_
#define GRIDFORMAT_TEST_VTK_WRITER_TESTER_HPP_

#include <vector>
#include <string>
#include <utility>
#include <type_traits>

#include <gridformat/common/precision.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/grid/type_traits.hpp>

#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"


namespace GridFormat::Test::VTK {

using namespace GridFormat::Encoding;
using namespace GridFormat::Compression;
using namespace GridFormat::VTK::DataFormat;

template<typename T>
std::string _name(const T& t) { return GridFormat::VTK::attribute_name(t); }
std::string _name(const None&) { return "none"; }
std::string _name(const Automatic&) { return "auto"; }
std::string _name(const Inlined&) { return "inlined"; }
std::string _name(const Appended&) { return "appended"; }

template<typename T>
struct Dimension;
template<typename T> requires(is_complete<GridFormat::GridTypeTraitsDetail::Dimension<T>>)
struct Dimension<T> : public std::integral_constant<int, dimension<T>> {};
template<int dim, int space_dim>
struct Dimension<UnstructuredGrid<dim, space_dim>> : public std::integral_constant<int, dim> {};


template<typename Grid>
class WriterTester {

    static constexpr int dim = Dimension<Grid>::value;
    static constexpr int space_dim = space_dimension<Grid>;

    template<typename T>
    static constexpr bool is_writer_factory = std::is_invocable_v<
        T,
        const Grid&,
        const GridFormat::VTK::XMLOptions&
    >;

 public:
    explicit WriterTester(Grid&& grid,
                          std::string extension,
                          bool verbose = true,
                          std::string suffix = "")
    : _grid{std::move(grid)}
    , _extension{std::move(extension)}
    , _prefix{_extension.substr(1)}
    , _suffix{std::move(suffix)}
    , _verbose{verbose} {
        _xml_options.push_back({.encoder = ascii, .compressor = none, .data_format = inlined});
        _xml_options.push_back({.encoder = base64, .compressor = none, .data_format = inlined});
        _xml_options.push_back({.encoder = base64, .compressor = none, .data_format = appended});
        _xml_options.push_back({.encoder = raw, .compressor = none, .data_format = appended});

        // for compressors, use small block size such that multiple blocks are compressed/written
#if GRIDFORMAT_HAVE_LZ4
        _xml_options.push_back({.encoder = raw, .compressor = lz4.with({.block_size = 100}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = lz4, .data_format = appended});
#endif
#if GRIDFORMAT_HAVE_LZMA
        _xml_options.push_back({.encoder = raw, .compressor = lzma.with({.block_size = 100}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = lzma, .data_format = appended});
#endif
#if GRIDFORMAT_HAVE_ZLIB
        _xml_options.push_back({.encoder = raw, .compressor = zlib.with({.block_size = 100}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = zlib, .data_format = appended});
        // this should raise a warning but still work
        _xml_options.push_back({.encoder = ascii, .compressor = zlib, .data_format = inlined});
#endif
    }

    template<typename Factory> requires(is_writer_factory<Factory>)
    void test(const Factory& factory) const {
        {  // default opts
            GridFormat::VTK::XMLOptions opts{};
            auto writer = factory(_grid, opts);
            write_test_file<space_dim>(writer, _make_filename(opts), {}, _verbose);
        }
        {  // custom header
            GridFormat::VTK::XMLOptions opts{.header_precision = uint32};
            auto writer = factory(_grid, opts);
            write_test_file<space_dim>(writer, _add_header_prec_suffix(_make_filename(opts), uint32), {}, _verbose);
        }
        {  // custom coordinate precision
            GridFormat::VTK::XMLOptions opts{.coordinate_precision = float32};
            auto writer = factory(_grid, opts);
            write_test_file<space_dim>(writer, _add_coord_prec_suffix(_make_filename(opts), float32), {}, _verbose);
        }
        {  // custom field precision
            GridFormat::VTK::XMLOptions opts{.coordinate_precision = float32};
            auto writer = factory(_grid, opts);
            write_test_file<space_dim>(writer, _add_coord_prec_suffix(_make_filename(opts), float32), {}, _verbose);
        }
        {  // test invalid combinations
            auto writer = factory(_grid, GridFormat::VTK::XMLOptions{});
            _check_failure_with_invalid_opts(writer);
        }
        std::ranges::for_each(_xml_options, [&] (const GridFormat::VTK::XMLOptions& _opts) {
            auto writer = factory(_grid, GridFormat::VTK::XMLOptions{})
                            .with_encoding(GridFormat::Variant::without<Automatic>(_opts.encoder))
                            .with_compression(GridFormat::Variant::without<Automatic>(_opts.compressor))
                            .with_data_format(GridFormat::Variant::without<Automatic>(_opts.data_format));
            write_test_file<space_dim>(writer, _make_filename(_opts) + "_modified", {}, _verbose);
        });
    }

 private:
    template<typename Writer>
    void _check_failure_with_invalid_opts(Writer& w) const {
        GridFormat::VTK::XMLOptions ascii_appended;
        ascii_appended.encoder = Encoding::ascii;
        ascii_appended.data_format = GridFormat::VTK::DataFormat::appended;
        try { w.with(ascii_appended).write("should_fail"); }
        catch (const GridFormat::ValueError& e) {}

        GridFormat::VTK::XMLOptions raw_inline;
        raw_inline.encoder = Encoding::raw;
        raw_inline.data_format = GridFormat::VTK::DataFormat::inlined;
        try { w.with(raw_inline).write("should_fail"); }
        catch (const GridFormat::ValueError& e) {}
    }

    template<typename T>
    std::string _add_header_prec_suffix(const std::string& name, const Precision<T>& p) const {
        return name +"_headerprecision_" + _name(DynamicPrecision{p});
    }

    template<typename T>
    std::string _add_coord_prec_suffix(const std::string& name, const Precision<T>& p) const {
        return name + "_coordprecision_" + _name(DynamicPrecision{p});
    }

    std::string _make_filename(const GridFormat::VTK::XMLOptions& opts) const {
        std::string result = _prefix + "_" + std::to_string(dim) + "d_in_" + std::to_string(space_dim) + "d";
        result += "_encoder_";
        result += std::visit([] (const auto& e) { return _name(e); }, opts.encoder);
        result += "_compressor_";
        result += std::visit([] (const auto& c) { return _name(c); }, opts.compressor);
        result += "_format_";
        result += std::visit([] (const auto& f) { return _name(f); }, opts.data_format);
        result += (_suffix.empty() ? "" : "_" + _suffix);
        return result;
    }

    Grid _grid;
    std::string _extension;
    std::string _prefix;
    std::string _suffix;
    bool _verbose;
    std::vector<GridFormat::VTK::XMLOptions> _xml_options;
};

}  // namespace GridFormat::Test::VTK

#endif  // GRIDFORMAT_TEST_VTK_WRITER_TESTER_HPP_
