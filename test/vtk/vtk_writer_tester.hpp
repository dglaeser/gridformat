// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_VTK_WRITER_TESTER_HPP_
#define GRIDFORMAT_TEST_VTK_WRITER_TESTER_HPP_

#include <vector>
#include <string>
#include <utility>
#include <type_traits>

#include <gridformat/common/precision.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/vtk/attributes.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"


namespace GridFormat::Test::VTK {

template<typename T>
std::string _name(const T& t) { return GridFormat::VTK::attribute_name(t); }
std::string _name(const GridFormat::None&) { return "none"; }
std::string _name(const GridFormat::Automatic&) { return "auto"; }
std::string _name(const GridFormat::VTK::DataFormat::Inlined&) { return "inlined"; }
std::string _name(const GridFormat::VTK::DataFormat::Appended&) { return "appended"; }

template<typename T>
struct SpaceDimension;
template<int dim>
struct SpaceDimension<StructuredGrid<dim>> : public std::integral_constant<int, dim> {};
template<int dim>
struct SpaceDimension<OrientedStructuredGrid<dim>> : public std::integral_constant<int, dim> {};
template<int dim, int space_dim>
struct SpaceDimension<UnstructuredGrid<dim, space_dim>> : public std::integral_constant<int, space_dim> {};

template<typename T>
struct Dimension;
template<int dim>
struct Dimension<StructuredGrid<dim>> : public std::integral_constant<int, dim> {};
template<int dim>
struct Dimension<OrientedStructuredGrid<dim>> : public std::integral_constant<int, dim> {};
template<int dim, int space_dim>
struct Dimension<UnstructuredGrid<dim, space_dim>> : public std::integral_constant<int, dim> {};

using namespace GridFormat::Encoding;
using namespace GridFormat::Compression;
using namespace GridFormat::VTK::DataFormat;
using GridFormat::none;

template<typename Grid>
class WriterTester {

    static constexpr int dim = Dimension<Grid>::value;
    static constexpr int space_dim = SpaceDimension<Grid>::value;

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
        static constexpr std::size_t block_size = 100;
#if GRIDFORMAT_HAVE_LZ4
        _xml_options.push_back({.encoder = raw, .compressor = lz4.with({.block_size = block_size}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = lz4, .data_format = appended});
#endif
#if GRIDFORMAT_HAVE_LZMA
        _xml_options.push_back({.encoder = raw, .compressor = lzma.with({.block_size = block_size}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = lzma, .data_format = appended});
#endif
#if GRIDFORMAT_HAVE_ZLIB
        _xml_options.push_back({.encoder = raw, .compressor = zlib.with({.block_size = block_size}), .data_format = appended});
        _xml_options.push_back({.encoder = base64, .compressor = zlib, .data_format = appended});
        // this should raise a warning but still work
        _xml_options.push_back({.encoder = ascii, .compressor = zlib, .data_format = inlined});
#endif
    }

    template<typename Factory> requires(is_writer_factory<Factory>)
    void test(const Factory& factory) const {
        _test_default(factory);
        _test_custom_field_precision(factory);
        _test_all_options(factory);
    }

 private:
    template<typename Factory> requires(is_writer_factory<Factory>)
    void _test_default(const Factory& factory) const {
        GridFormat::VTK::XMLOptions opts;
        auto writer = factory(_grid, opts);
        const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(_grid);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<double>{});
        GridFormat::Test::add_meta_data(writer);
        _write_with(writer, opts);
        _write_with_header(writer, opts, GridFormat::uint32);
        _write_with_coordprec(writer, opts, GridFormat::float32);
        _check_failure_with_invalid_opts(writer);
    }

    template<typename Factory> requires(is_writer_factory<Factory>)
    void _test_all_options(const Factory& factory) const {
        auto writer = factory(_grid, GridFormat::VTK::XMLOptions{});
        const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(_grid);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<double>{});
        std::ranges::for_each(_xml_options, [&] (const GridFormat::VTK::XMLOptions& _opts) {
            auto cpy = writer.with_encoding(GridFormat::Variant::without<Automatic>(_opts.encoder))
                             .with_compression(GridFormat::Variant::without<Automatic>(_opts.compressor))
                             .with_data_format(GridFormat::Variant::without<Automatic>(_opts.data_format));
            GridFormat::Test::add_meta_data(cpy);
            _write_with(cpy, _opts, "_modified");
        });
    }

    template<typename Factory> requires(is_writer_factory<Factory>)
    void _test_custom_field_precision(const Factory& factory) const {
        GridFormat::VTK::XMLOptions opts;
        auto writer = factory(_grid, opts);
        const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(_grid);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
        GridFormat::Test::add_meta_data(writer);
        _write(writer, _add_field_prec_suffix(_make_filename(opts), GridFormat::Precision<float>{}));
    }

    template<typename Writer, typename T>
    void _write_with_header(Writer& w,
                            GridFormat::VTK::XMLOptions opts,
                            const Precision<T>& p) const {
        opts.header_precision = p;
        _write(w.with(opts), _add_header_prec_suffix(_make_filename(opts), p));
    }

    template<typename Writer, typename T>
    void _write_with_coordprec(Writer& w,
                               GridFormat::VTK::XMLOptions opts,
                               const Precision<T>& p) const {
        opts.coordinate_precision = p;
        _write(w.with(opts), _add_coord_prec_suffix(_make_filename(opts), p));
    }

    template<typename Writer>
    void _write_with(Writer& w,
                     const GridFormat::VTK::XMLOptions& opts,
                     const std::string& suffix = "") const {
        _write(w.with(opts), _make_filename(opts) + suffix);
    }

    template<typename Writer>
    void _write(const Writer& w, const std::string& filename) const {
        w.write(filename);
        if (_verbose)
            std::cout << GridFormat::as_highlight("Wrote '" + filename + _extension + "'") << std::endl;
    }

    template<typename Writer>
    void _check_failure_with_invalid_opts(Writer& w) const {
        GridFormat::VTK::XMLOptions ascii_appended;
        ascii_appended.encoder = Encoding::ascii;
        ascii_appended.data_format = GridFormat::VTK::DataFormat::appended;
        try { w.with(ascii_appended).write("should_fail"); }
        catch (const GridFormat::ValueError& e) {}

        GridFormat::VTK::XMLOptions raw_inline;
        ascii_appended.encoder = Encoding::raw;
        ascii_appended.data_format = GridFormat::VTK::DataFormat::inlined;
        try { w.with(ascii_appended).write("should_fail"); }
        catch (const GridFormat::ValueError& e) {}
    }

    template<typename T>
    std::string _add_field_prec_suffix(const std::string& name, const Precision<T>& p) const {
        return name + "_fieldprecision_" + _name(DynamicPrecision{p});
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
