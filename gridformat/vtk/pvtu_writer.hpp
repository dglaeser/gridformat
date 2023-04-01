// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTUWriter
 */
#ifndef GRIDFORMAT_VTK_PVTU_WRITER_HPP_
#define GRIDFORMAT_VTK_PVTU_WRITER_HPP_

#include <ostream>
#include <string>
#include <fstream>
#include <cmath>

#include <gridformat/common/field.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/grid.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for parallel .pvtu files
 */
template<Concepts::UnstructuredGrid Grid,
         Concepts::Communicator Communicator>
class PVTUWriter : public VTK::XMLWriterBase<Grid, PVTUWriter<Grid, Communicator>> {
    using ParentType = VTK::XMLWriterBase<Grid, PVTUWriter<Grid, Communicator>>;

 public:
    explicit PVTUWriter(const Grid& grid,
                        Communicator comm,
                        VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".pvtu", xml_opts)
    , _comm(comm)
    {}

    PVTUWriter with(VTK::XMLOptions xml_opts) const {
        return PVTUWriter{this->grid(), _comm, std::move(xml_opts)};
    }

 private:
    Communicator _comm;

    void _write(std::ostream&) const override {
        throw InvalidState(
            "PVTUWriter does not support direct export into stream. "
            "Use overload with filename instead!"
        );
    }

    virtual void _write(const std::string& filename_with_ext) const {
        _write_piece(filename_with_ext);
        Parallel::barrier(_comm);  // ensure all pieces finished successfully
        if (Parallel::rank(_comm) == 0)
            _write_pvtu_file(filename_with_ext);
        Parallel::barrier(_comm);  // ensure .pvtu file is written before returning
    }

    void _write_piece(const std::string& par_filename) const {
        VTUWriter writer{this->grid(), this->_xml_opts()};
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            writer.set_point_field(name, VTK::make_vtk_field(this->_get_shared_point_field(name)));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            writer.set_cell_field(name, this->_get_shared_cell_field(name));
        });
        writer.write(_piece_filename(par_filename));
    }

    std::string _piece_filename(const std::string& par_filename) const {
        return _piece_filename(par_filename, Parallel::rank(_comm));
    }

    std::string _piece_filename(const std::string& par_filename, int rank) const {
        const std::string base_name = par_filename.substr(0, par_filename.find_last_of("."));
        return base_name + "-" + std::to_string(rank);
    }

    void _write_pvtu_file(const std::string& filename_with_ext) const {
        std::ofstream file_stream(filename_with_ext, std::ios::out);

        XMLElement pvtk_xml("VTKFile");
        pvtk_xml.set_attribute("type", "PUnstructuredGrid");

        XMLElement& grid = pvtk_xml.add_child("PUnstructuredGrid");
        XMLElement& ppoint_data = grid.add_child("PPointData");
        XMLElement& pcell_data = grid.add_child("PCellData");

        const auto _add_pdata_array = [&] (XMLElement& section,
                                            const std::string& name,
                                            const Field& field) {
            static constexpr std::size_t vtk_space_dim = 3;
            XMLElement& arr = section.add_child("PDataArray");
            arr.set_attribute("Name", name);
            arr.set_attribute("type", VTK::attribute_name(field.precision()));
            std::visit([&] (const auto& data_format) {
                std::visit([&] (const auto& encoder) {
                    arr.set_attribute("format", VTK::data_format_name(encoder, data_format));
                }, this->_xml_settings.encoder);
            }, this->_xml_settings.data_format);
            arr.set_attribute(
                "NumberOfComponents",
                static_cast<std::size_t>(std::pow(vtk_space_dim, field.layout().dimension() - 1))
            );
        };
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            _add_pdata_array(ppoint_data, name, this->_get_point_field(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            _add_pdata_array(pcell_data, name, this->_get_cell_field(name));
        });

        XMLElement& point_array = grid.add_child("PPoints").add_child("PDataArray");
        point_array.set_attribute("NumberOfComponents", "3");
        std::visit([&] <typename T> (const Precision<T>& prec) {
            point_array.set_attribute("type", VTK::attribute_name(prec));
        }, this->_xml_settings.coordinate_precision);

        for (int rank : std::views::iota(0, Parallel::size(_comm)))
            grid.add_child("Piece").set_attribute(
                "Source", _piece_filename(filename_with_ext, rank) + ".vtu"
            );

        write_xml_with_version_header(pvtk_xml, file_stream, Indentation{{.width = 2}});
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTU_WRITER_HPP_
