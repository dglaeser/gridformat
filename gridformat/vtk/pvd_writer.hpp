// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVDWriter
 */
#ifndef GRIDFORMAT_VTK_PVD_WRITER_HPP_
#define GRIDFORMAT_VTK_PVD_WRITER_HPP_

#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>

#include <gridformat/xml/element.hpp>
#include <gridformat/grid.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .pvd time-series file format
 */
template<typename VTKWriter>
class PVDWriter {
 public:
    explicit PVDWriter(VTKWriter&& writer, std::string base_filename)
    : _vtk_writer{std::move(writer)}
    , _base_filename{std::move(base_filename)}
    , _pvd_filename{_base_filename + ".pvd"}
    , _xml{"VTKFile"} {
        _xml.set_attribute("type", "Collection");
        _xml.set_attribute("version", "1.0");
        _xml.add_child("Collection");
    }

    std::string write(const Concepts::Scalar auto _time) {
        const auto time_step_index = _xml.get_child("Collection").num_children();
        const std::string vtk_filename = _vtk_writer.write(
            _base_filename + "-" + _get_file_number_string(time_step_index)
        );
        _add_dataset(_time, vtk_filename);
        std::ofstream pvd_file(_pvd_filename, std::ios::out);
        write_xml_with_version_header(_xml, pvd_file, Indentation{{.width = 2}});
        return _pvd_filename;
    }

    template<typename... Args>
    void set_point_field(Args&&... args) {
        _vtk_writer.set_point_field(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void set_cell_field(Args&&... args) {
        _vtk_writer.set_cell_field(std::forward<Args>(args)...);
    }

 private:
    std::string _get_file_number_string(const std::integral auto index) const {
        std::ostringstream file_number;
        file_number << std::setw(5) << std::setfill('0') << index;
        return file_number.str();
    }

    XMLElement& _add_dataset(const Concepts::Scalar auto _time, const std::string& filename) {
        auto& dataset = _xml.get_child("Collection").add_child("DataSet");
        dataset.set_attribute("timestep", _time);
        dataset.set_attribute("group", "");
        dataset.set_attribute("part", "0");
        dataset.set_attribute("name", "");
        dataset.set_attribute("file", filename);
        return dataset;
    }

    VTKWriter _vtk_writer;
    std::string _base_filename;
    std::string _pvd_filename;
    XMLElement _xml;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVD_WRITER_HPP_