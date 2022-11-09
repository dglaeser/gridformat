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
#include <gridformat/common/ranges.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::Grid Grid>
class XMLWriterBase : public WriterBase {
    static constexpr std::size_t vtk_space_dim = 3;

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
        WriterBase::set_point_field( name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_point_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_point_field( name, _make_tensor_range(std::forward<V>(v)), prec);
    }

    //! Vectors need to be made 3d for VTK
    template<Concepts::Vectors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field( name, _make_vector_range(std::forward<V>(v)), prec);
    }

    //! Tensors need to be made 3d for VTK
    template<Concepts::Tensors V, Concepts::Scalar T = MDRangeScalar<V>>
    void set_cell_field(const std::string& name, V&& v, const Precision<T>& prec = {}) {
        WriterBase::set_cell_field( name, _make_tensor_range(std::forward<V>(v)), prec);
    }

 private:
    template<Concepts::Vectors V>
    auto _make_vector_range(V&& v) {
        return std::forward<V>(v) | std::views::transform([] <std::ranges::range R> (R&& r) {
            return make_extended<vtk_space_dim>(std::forward<R>(r));
        });
    }

    template<Concepts::Tensors T>
    auto _make_tensor_range(T&& t) {
        return std::forward<T>(t) | std::views::transform([] <std::ranges::range Tensor> (Tensor outer) {
            using Vector = std::ranges::range_value_t<Tensor>;
            using Scalar = std::ranges::range_value_t<Vector>;
            static_assert(Concepts::StaticallySized<Vector> && "Tensor expansion expects statically sized tensor rows");

            Vector last_row;
            std::ranges::fill(last_row, Scalar{0.0});
            auto extended_tensor = make_extended<vtk_space_dim>(std::move(outer), std::move(last_row));
            return std::ranges::owning_view{std::move(extended_tensor)}
                | std::views::transform([] <typename V> (V&& vector) {
                    return make_extended<vtk_space_dim>(std::forward<V>(vector), Scalar{0.0});
            });
        });
    }

    const Grid& _grid;
    PrecisionTraits _header_precision = Precision<std::size_t>{};
    PrecisionTraits _coordinate_precision = Precision<CoordinateType<Grid>>{};
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_XML_WRITER_BASE_HPP_