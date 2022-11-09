// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_VTU_WRITER_HPP_
#define GRIDFORMAT_VTK_VTU_WRITER_HPP_

#include <ranges>
#include <ostream>

#include <gridformat/common/extended_range.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/xml_writer.hpp>
#include <gridformat/vtk/options.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::UnstructuredGrid Grid>
class VTUWriter : public VTK::XMLWriterBase<Grid> {
    using ParentType = VTK::XMLWriterBase<Grid>;

 public:
    explicit VTUWriter(const Grid& grid) : ParentType(grid) {}

 private:
    void _write(std::ostream& s) const override {
        for (const auto& n : this->_point_field_names()) {
            s << "N = " << n << std::endl;
            s << " -> size: " << this->_get_point_field(n).layout().number_of_entries() << std::endl;
            auto ser = this->_get_point_field(n).serialized();
            s << " -> Data: ";
            const double* data = reinterpret_cast<const double*>(ser.data());
            for (std::size_t i = 0; i < this->_get_point_field(n).layout().number_of_entries(); ++i)
                s << data[i] << ",";
            s << "\n";
        }

        for (const auto& n : this->_cell_field_names()) {
            s << "N = " << n << std::endl;
            s << " -> size: " << this->_get_cell_field(n).layout().number_of_entries() << std::endl;
            auto ser = this->_get_cell_field(n).serialized();
            s << " -> Data: ";
            const double* data = reinterpret_cast<const double*>(ser.data());
            for (std::size_t i = 0; i < this->_get_cell_field(n).layout().number_of_entries(); ++i)
                s << data[i] << ",";
            s << "\n";
        }
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_