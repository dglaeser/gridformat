#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrecision = GridFormat::Precision<double>>
void write([[maybe_unused]] const XMLOptions& xml_opts,
           [[maybe_unused]] const PrecisionOptions& prec_opts,
           [[maybe_unused]] const std::string& filename,
           [[maybe_unused]] const FieldPrecision& prec = {}) {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    auto point_scalars = GridFormat::Test::make_point_data<double>(grid);
    auto point_vectors = GridFormat::Test::make_vector_data(point_scalars);
    auto point_tensors = GridFormat::Test::make_tensor_data(point_scalars);
    auto cell_scalars = GridFormat::Test::make_cell_data<double>(grid);
    auto cell_vectors = GridFormat::Test::make_vector_data(cell_scalars);
    auto cell_tensors = GridFormat::Test::make_tensor_data(cell_scalars);

    GridFormat::VTPWriter writer{grid, xml_opts, prec_opts};
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

    std::cout << "Wrote " << writer.write(filename) << ".vtu" << std::endl;
}

int main() {
    write(GridFormat::VTK::XMLOptions{}, GridFormat::VTK::PrecisionOptions{}, "vtp_default");
    write(
        GridFormat::VTK::XMLOptions{.encoder = GridFormat::Encoding::ascii},
        GridFormat::VTK::PrecisionOptions{},
        "vtp_ascii"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtp_base64_inlined"
    );
#if GRIDFORMAT_HAVE_LZMA
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compressor = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtp_base64_inlined_lzma_compression"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compressor = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtp_base64_inlined_lzma_compression_custom_field_precision",
        GridFormat::Precision<float>{}
    );
#endif  // GRIDFORMAT_HAVE_LZMA

    return 0;
}
