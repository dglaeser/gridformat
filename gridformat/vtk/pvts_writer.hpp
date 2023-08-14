// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTSWriter
 */
#ifndef GRIDFORMAT_VTK_PVTS_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTS_WRITER_HPP_

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
#include <gridformat/vtk/vts_writer.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvts files
 */
template<Concepts::StructuredGrid Grid,
         Concepts::Communicator Communicator>
class PVTSWriter : public VTK::XMLWriterBase<Grid, PVTSWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTSWriter<Grid, Communicator>>;
    using CT = CoordinateType<Grid>;

    static constexpr std::size_t space_dim = 3;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr int root_rank = 0;

 public:
    explicit PVTSWriter(LValueReferenceOf<const Grid> grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".pvts", true, xml_opts)
    , _comm(comm)
    {}

    const Communicator& communicator() const {
        return _comm;
    }

 private:
    Communicator _comm;

    PVTSWriter _with(VTK::XMLOptions xml_opts) const override {
        return PVTSWriter{this->grid(), _comm, std::move(xml_opts)};
    }

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTSWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const override {
        const auto& local_extents = extents(this->grid());
        const auto [origin, is_negative_axis] = _get_origin_and_orientations(local_extents);

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
            _write_pvts_file(filename_with_ext, my_whole_extent, exts_begin, exts_end);
        Parallel::barrier(_comm);  // ensure .pvts file is written before returning
    }

    auto _get_origin_and_orientations(const std::ranges::range auto& extents) const {
        auto is_negative_axis = Ranges::filled_array<dim>(false);
        std::ranges::for_each(extents, [&, dir=0] (const auto& e) mutable {
            if (e > 0)
                is_negative_axis[dir] = _is_negative_axis(dir);
            dir++;
        });

        std::array<CT, dim> origin;
        static constexpr auto origin_loc = _origin_location();
        for (const auto& p : points(this->grid())) {
            const auto& loc = location(this->grid(), p);
            const auto& pos = coordinates(this->grid(), p);
            if (std::ranges::equal(loc, origin_loc)) {
                std::ranges::copy(pos, origin.begin());
                return std::make_tuple(std::move(origin), std::move(is_negative_axis));
            }
        }

        throw InvalidState("Could not determine origin");
    }

    static constexpr auto _origin_location() {
        std::array<std::size_t, dim> origin;
        std::ranges::fill(origin, 0);
        return origin;
    }

    bool _is_negative_axis(unsigned axis) const {
        for (const auto& p0 : points(this->grid())) {
            const auto i0 = Ranges::at(axis, location(this->grid(), p0));
            const auto x0 = Ranges::at(axis, coordinates(this->grid(), p0));
            for (const auto& p1 : points(this->grid())) {
                const auto i1 = Ranges::at(axis, location(this->grid(), p1));
                const auto x1 = Ranges::at(axis, coordinates(this->grid(), p1));
                if (i1 > i0)
                    return x1 < x0;
            }
        }
        throw InvalidState("Could not determine axis orientation");
    }

    void _write_piece(const std::string& par_filename,
                      const std::array<std::size_t, dim>& offset,
                      typename VTSWriter<Grid>::Domain domain) const {
        auto writer = VTSWriter{this->grid(), this->_xml_opts}
                        .as_piece_for(std::move(domain))
                        .with_offset(offset);
        this->copy_fields(writer);
        writer.write(PVTK::piece_basefilename(par_filename, Parallel::rank(_comm)));
    }

    void _write_pvts_file(const std::string& filename_with_ext,
                          const std::array<std::size_t, dim>& extents,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_begin,
                          const std::vector<std::array<std::size_t, dim>>& proc_extents_end) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PStructuredGrid");

        XMLElement& grid = pvtk_xml.add_child("PStructuredGrid");
        grid.set_attribute("WholeExtent", VTK::CommonDetail::extents_string(extents));

        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");
        XMLElement& pcoords = grid.add_child("PPoints");
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
                    XMLElement& pdata_array = pcoords.add_child("PDataArray");
                    pdata_array.set_attribute("NumberOfComponents", std::to_string(space_dim));
                    pdata_array.set_attribute("Name", "Coordinates");
                    pdata_array.set_attribute("format", VTK::data_format_name(encoder, data_format));
                    pdata_array.set_attribute("type", VTK::attribute_name(prec));
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
                PVTK::piece_basefilename(filename_with_ext, rank) + ".vts"
            }.filename());
        });

        this->_set_default_active_fields(pvtk_xml.get_child("PStructuredGrid"));
        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

template<typename G, Concepts::Communicator C>
PVTSWriter(G&&, const C&, VTK::XMLOptions = {}) -> PVTSWriter<std::remove_cvref_t<G>, C>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTS_WRITER_HPP_
