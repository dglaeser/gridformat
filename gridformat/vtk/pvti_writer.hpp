// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTIWriter
 */
#ifndef GRIDFORMAT_VTK_PVTI_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTI_WRITER_HPP_

#include <ostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <array>
#include <tuple>
#include <cmath>

#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/lvalue_reference.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/helpers.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/parallel.hpp>
#include <gridformat/vtk/vti_writer.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvti files
 */
template<Concepts::ImageGrid Grid,
         Concepts::Communicator Communicator>
class PVTIWriter : public VTK::XMLWriterBase<Grid, PVTIWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTIWriter<Grid, Communicator>>;
    using CT = CoordinateType<Grid>;

    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr int root_rank = 0;

 public:
    explicit PVTIWriter(LValueReferenceOf<const Grid> grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".pvti", true, xml_opts)
    , _comm(comm)
    {}

    const Communicator& communicator() const {
        return _comm;
    }

 private:
    Communicator _comm;

    PVTIWriter _with(VTK::XMLOptions xml_opts) const override {
        return PVTIWriter{this->grid(), _comm, std::move(xml_opts)};
    }

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTIWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const override {
        const auto& local_origin = origin(this->grid());
        const auto& local_extents = extents(this->grid());

        PVTK::StructuredParallelGridHelper helper{_comm};
        const auto all_origins = Parallel::gather(_comm, local_origin, root_rank);
        const auto all_extents = Parallel::gather(_comm, local_extents, root_rank);
        const auto is_negative_axis = VTK::CommonDetail::structured_grid_axis_orientation(spacing(this->grid()));
        const auto [exts_begin, exts_end, whole_extent, origin] = helper.compute_extents_and_origin(
            all_origins,
            all_extents,
            is_negative_axis,
            basis(this->grid())
        );

        const auto my_whole_extent = Parallel::broadcast(_comm, whole_extent, root_rank);
        const auto my_whole_origin = Parallel::broadcast(_comm, origin, root_rank);
        const auto my_extent_offset = Parallel::scatter(_comm, Ranges::flat(exts_begin), root_rank);

        _write_piece(filename_with_ext, Ranges::to_array<dim>(my_extent_offset), {my_whole_origin, my_whole_extent});
        Parallel::barrier(_comm);  // ensure all pieces finished successfully
        if (Parallel::rank(_comm) == 0)
            _write_pvti_file(filename_with_ext, my_whole_origin, my_whole_extent, exts_begin, exts_end);
        Parallel::barrier(_comm);  // ensure .pvti file is written before returning
    }

    void _write_piece(const std::string& par_filename,
                      const std::array<std::size_t, dim>& offset,
                      typename VTIWriter<Grid>::Domain domain) const {
        auto writer = VTIWriter{this->grid(), this->_xml_opts}
                        .as_piece_for(std::move(domain))
                        .with_offset(offset);
        this->copy_fields(writer);
        writer.write(PVTK::piece_basefilename(par_filename, Parallel::rank(_comm)));
    }

    void _write_pvti_file(const std::string& filename_with_ext,
                          const std::array<CT, dim>& origin,
                          const std::array<std::size_t, dim>& extents,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_begin,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_end) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PImageData");

        XMLElement& grid = pvtk_xml.add_child("PImageData");
        grid.set_attribute("WholeExtent", VTK::CommonDetail::extents_string(extents));
        grid.set_attribute("Origin", VTK::CommonDetail::number_string_3d(origin));
        grid.set_attribute("Spacing", VTK::CommonDetail::number_string_3d(spacing(this->grid())));
        grid.set_attribute("Direction", VTK::CommonDetail::direction_string(basis(this->grid())));

        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");
        std::visit([&] (const auto& encoder) {
            std::visit([&] (const auto& data_format) {
                PVTK::PDataArrayHelper pdata_helper{encoder, data_format, ppoint_data};
                PVTK::PDataArrayHelper cdata_helper{encoder, data_format, pcell_data};
                std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
                    pdata_helper.add(name, this->_get_point_field(name));
                });
                std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
                    cdata_helper.add(name, this->_get_cell_field(name));
                });
            }, this->_xml_settings.data_format);
        }, this->_xml_settings.encoder);

        std::ranges::for_each(Parallel::ranks(_comm), [&] (int rank) {
            std::array<std::size_t, dim> extents_begin;
            std::array<std::size_t, dim> extents_end;
            for (unsigned dir = 0; dir < dim; ++dir) {
                extents_begin[dir] = proc_extents_begin[rank][dir];
                extents_end[dir] = proc_extents_end[rank][dir];
            }

            auto& piece = grid.add_child("Piece");
            piece.set_attribute("Extent", VTK::CommonDetail::extents_string(extents_begin, extents_end));
            piece.set_attribute("Source", std::filesystem::path{
                PVTK::piece_basefilename(filename_with_ext, rank) + ".vti"
            }.filename());
        });

        this->_set_default_active_fields(pvtk_xml.get_child("PImageData"));
        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

template<typename G, Concepts::Communicator C>
PVTIWriter(G&&, const C&, VTK::XMLOptions = {}) -> PVTIWriter<std::remove_cvref_t<G>, C>;

namespace Traits {

template<typename... Args>
struct WritesConnectivity<PVTIWriter<Args...>> : public std::false_type {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTI_WRITER_HPP_
