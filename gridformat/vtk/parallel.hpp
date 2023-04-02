// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Helper function for writing parallel VTK files
 */
#ifndef GRIDFORMAT_VTK_PARALLEL_HPP_
#define GRIDFORMAT_VTK_PARALLEL_HPP_

#include <string>
#include <cmath>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/attributes.hpp>

namespace GridFormat::PVTK {

//! Return the piece filename (w/o extension) for the given rank
 std::string piece_basefilename(const std::string& par_filename, int rank) {
    const std::string base_name = par_filename.substr(0, par_filename.find_last_of("."));
    return base_name + "-" + std::to_string(rank);
}

//! Helper to add a PDataArray child to an xml element
template<typename Encoder, typename DataFormat>
class PDataArrayHelper {
 public:
    PDataArrayHelper(const Encoder& e,
                     const DataFormat& df,
                     XMLElement& element)
    : _encoder(e)
    , _data_format(df)
    , _element(element)
    {}

    void add(const std::string& name, const Field& field) {
        // vtk always uses 3d, this assumes that the field
        // is transformed accordingly in the piece writers
        static constexpr std::size_t vtk_space_dim = 3;
        const auto ncomps = std::pow(vtk_space_dim, field.layout().dimension() - 1);

        XMLElement& arr = _element.add_child("PDataArray");
        arr.set_attribute("Name", name);
        arr.set_attribute("type", VTK::attribute_name(field.precision()));
        arr.set_attribute("format", VTK::data_format_name(_encoder, _data_format));
        arr.set_attribute("NumberOfComponents", static_cast<std::size_t>(ncomps));
    }

 private:
    const Encoder& _encoder;
    const DataFormat& _data_format;
    XMLElement& _element;
};

}  // namespace GridFormat::PVTK

#endif  // GRIDFORMAT_VTK_PARALLEL_HPP_
