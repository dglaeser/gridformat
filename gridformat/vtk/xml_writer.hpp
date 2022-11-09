// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_
#define GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_

#include <gridformat/common/precision.hpp>
#include <gridformat/common/writer.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::Grid Grid>
class XMLWriterBase : public WriterBase {
 public:
    explicit XMLWriterBase(const Grid& grid)
    : _grid(grid)
    {}

    template<typename T>
    void set_header_precision(const Precision<T>& prec) {
        _header_precision = prec;
    }

    template<typename T>
    void set_coordinate_precision(const Precision<T>& prec) {
        _coordinate_precision = prec;
    }

    using WriterBase::set_point_field;
    using WriterBase::set_cell_field;

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field(
            name,
            std::forward<V>(v) | std::views::transform([] <std::ranges::range R> (R&& r) {
                return make_extended<3>(std::forward<R>(r));
            }),
            prec
        );
    }

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field(
            name,
            std::forward<V>(v) | std::views::transform([] <std::ranges::range R> (R&& r) {
                return make_extended<3>(std::forward<R>(r));
            }),
            prec
        );
    }

 private:
    const Grid& _grid;
    PrecisionTraits _header_precision = Precision<std::size_t>{};
    PrecisionTraits _coordinate_precision = Precision<CoordinateType<Grid>>{};
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_