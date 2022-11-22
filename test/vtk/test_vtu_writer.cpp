#include <vector>
#include <cmath>
#include <string>

#include <gridformat/compression/lzma.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<typename Writer, typename FieldPrecision>
void write_fields(Writer& writer,
                  const FieldPrecision& prec,
                  const std::string& filename) {
    const auto& grid = writer.grid();
    auto point_scalars = GridFormat::Test::make_point_data<double>(grid);
    auto point_vectors = GridFormat::Test::make_vector_data(point_scalars);
    auto point_tensors = GridFormat::Test::make_tensor_data(point_scalars);
    auto cell_scalars = GridFormat::Test::make_cell_data<double>(grid);
    auto cell_vectors = GridFormat::Test::make_vector_data(cell_scalars);
    auto cell_tensors = GridFormat::Test::make_tensor_data(cell_scalars);

    writer.set_point_field("pscalar", [&] (const auto& p) { return point_scalars[p.id]; });
    writer.set_point_field("pvector", [&] (const auto& p) { return point_vectors[p.id]; });
    writer.set_point_field("ptensor", [&] (const auto& p) { return point_tensors[p.id]; });
    writer.set_cell_field("cscalar", [&] (const auto& c) { return cell_scalars[c.id]; });
    writer.set_cell_field("cvector", [&] (const auto& c) { return cell_vectors[c.id]; });
    writer.set_cell_field("ctensor", [&] (const auto& c) { return cell_tensors[c.id]; });

    writer.set_point_field("pscalar_custom_prec", [&] (const auto& p) { return point_scalars[p.id]; }, prec);
    writer.set_point_field("pvector_custom_prec", [&] (const auto& p) { return point_vectors[p.id]; }, prec);
    writer.set_point_field("ptensor_custom_prec", [&] (const auto& p) { return point_tensors[p.id]; }, prec);
    writer.set_cell_field("cscalar_custom_prec", [&] (const auto& c) { return cell_scalars[c.id]; }, prec);
    writer.set_cell_field("cvector_custom_prec", [&] (const auto& c) { return cell_vectors[c.id]; }, prec);
    writer.set_cell_field("ctensor_custom_prec", [&] (const auto& c) { return cell_tensors[c.id]; }, prec);

    std::cout << "Writing " << filename << ".vtu" << std::endl;
    writer.write(filename);
}

template<typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrecision = GridFormat::Precision<double>>
void write(const XMLOptions& xml_opts,
           const PrecisionOptions& prec_opts,
           const std::string& filename,
           const FieldPrecision& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured_2d();
    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    write_fields(writer, prec, filename);
}

template<typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrecision = GridFormat::Precision<double>>
void write_from_abstract_base(const XMLOptions& xml_opts,
                              const PrecisionOptions& prec_opts,
                              const std::string& filename,
                              const FieldPrecision& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured_2d();
    using Base = GridFormat::GridWriter<decltype(grid)>;
    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    std::unique_ptr<Base> writer_ptr = std::make_unique<decltype(writer)>(std::move(writer));
    write_fields(*writer_ptr, prec, filename);
}

int main() {
    write(GridFormat::VTK::XMLOptions{}, GridFormat::VTK::PrecisionOptions{}, "vtu_default");
    write(
        GridFormat::VTK::XMLOptions{.encoder = GridFormat::Encoding::ascii},
        GridFormat::VTK::PrecisionOptions{},
        "vtu_ascii"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        "vtu_base64_inlined"
    );
    write_from_abstract_base(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        "vtu_base64_inlined_from_base_writer"
    );
#if GRIDFORMAT_HAVE_LZMA
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compression = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        "vtu_base64_inlined_lzma_compression_custom_header_precision"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compression = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtu_base64_inlined_lzma_compression_custom_field_precision",
        GridFormat::Precision<float>{}
    );
#endif  // GRIDFORMAT_HAVE_LZMA

    return 0;
}
