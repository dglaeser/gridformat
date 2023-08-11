// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTRWriter
 */
#ifndef GRIDFORMAT_VTK_PVTR_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTR_WRITER_HPP_

#include <ostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <array>
#include <tuple>

#include <gridformat/common/field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/lvalue_reference.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/helpers.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/parallel.hpp>
#include <gridformat/vtk/vtr_writer.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvtr files
 */
template<Concepts::RectilinearGrid Grid,
         Concepts::Communicator Communicator>
class PVTRWriter : public VTK::XMLWriterBase<Grid, PVTRWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTRWriter<Grid, Communicator>>;
    using CT = CoordinateType<Grid>;

    static constexpr std::size_t space_dim = 3;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr int root_rank = 0;

 public:
    explicit PVTRWriter(LValueReferenceOf<const Grid> grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".pvtr", true, xml_opts)
    , _comm(comm)
    {}

    const Communicator& communicator() const {
        return _comm;
    }

 private:
    Communicator _comm;

    PVTRWriter _with(VTK::XMLOptions xml_opts) const override {
        return PVTRWriter{this->grid(), _comm, std::move(xml_opts)};
    }

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTRWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const override {
        const auto& local_extents = extents(this->grid());
        const auto [origin, is_negative_axis] = _get_origin_and_orientations();

        PVTK::StructuredParallelGridHelper helper{_comm};
        const auto all_origins = Parallel::gather(_comm, origin, root_rank);
        const auto all_extents = Parallel::gather(_comm, local_extents, root_rank);
        const auto [exts_begin, exts_end, whole_extent, _] = helper.compute_extents_and_origin(
            all_origins,
            all_extents,
            is_negative_axis
        );

        const auto my_whole_extent = Parallel::broadcast(_comm, whole_extent, root_rank);
        const auto my_extent_offset = Parallel::scatter(_comm, Ranges::flat(exts_begin), root_rank);

        _write_piece(filename_with_ext, Ranges::to_array<dim>(my_extent_offset), {my_whole_extent});
        Parallel::barrier(_comm);  // ensure all pieces finished successfully
        if (Parallel::rank(_comm) == 0)
            _write_pvtr_file(filename_with_ext, my_whole_extent, exts_begin, exts_end);
        Parallel::barrier(_comm);  // ensure .pvtr file is written before returning
    }

    auto _get_origin_and_orientations() const {
        std::array<CT, dim> origin;
        std::array<bool, dim> is_negative_axis;
        for (unsigned dir = 0; dir < dim; ++dir) {
            std::array<CT, 2> ordinates_01{0, 0};
            const auto& dir_ordinates = ordinates(this->grid(), dir);
            std::ranges::copy(std::views::take(dir_ordinates, 2), ordinates_01.begin());
            origin[dir] = ordinates_01[0];
            is_negative_axis[dir] = ordinates_01[1] - ordinates_01[0] < CT{0};
        }
        return std::make_tuple(origin, is_negative_axis);
    }

    void _write_piece(const std::string& par_filename,
                      const std::array<std::size_t, dim>& offset,
                      typename VTRWriter<Grid>::Domain domain) const {
        auto writer = VTRWriter{this->grid(), this->_xml_opts}
                        .as_piece_for(std::move(domain))
                        .with_offset(offset);
        this->copy_fields(writer);
        writer.write(PVTK::piece_basefilename(par_filename, Parallel::rank(_comm)));
    }

    void _write_pvtr_file(const std::string& filename_with_ext,
                          const std::array<std::size_t, dim>& extents,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_begin,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_end) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PRectilinearGrid");

        XMLElement& grid = pvtk_xml.add_child("PRectilinearGrid");
        grid.set_attribute("WholeExtent", VTK::CommonDetail::extents_string(extents));

        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");
        XMLElement& pcoords = grid.add_child("PCoordinates");
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

                std::visit([&] <typename T> (const Precision<T>& prec) {
                    for (unsigned int i = 0; i < space_dim; ++i) {
                        XMLElement& pdata_array = pcoords.add_child("PDataArray");
                        pdata_array.set_attribute("NumberOfComponents", "1");
                        pdata_array.set_attribute("Name", "X_" + std::to_string(i));
                        pdata_array.set_attribute("format", VTK::data_format_name(encoder, data_format));
                        pdata_array.set_attribute("type", VTK::attribute_name(prec));
                    }
                }, this->_xml_settings.coordinate_precision);
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
                PVTK::piece_basefilename(filename_with_ext, rank) + ".vtr"
            }.filename());
        });

        this->_set_default_active_fields(pvtk_xml.get_child("PRectilinearGrid"));
        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

template<typename G, Concepts::Communicator C>
PVTRWriter(G&&, const C&, VTK::XMLOptions = {}) -> PVTRWriter<std::remove_cvref_t<G>, C>;

namespace Traits {

template<typename... Args>
struct WritesConnectivity<PVTRWriter<Args...>> : public std::false_type {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTR_WRITER_HPP_
