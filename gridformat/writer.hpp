// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_WRITER_HPP_
#define GRIDFORMAT_WRITER_HPP_

#include <memory>
#include <utility>

#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/parallel/concepts.hpp>

namespace GridFormat {

template<typename FileFormat>
struct WriterFactory;


#ifndef DOXYGEN
namespace Detail {

    template<typename FileFormat, typename Grid>
    static constexpr bool has_sequential_factory
        = is_complete<WriterFactory<FileFormat>>
        and requires(const FileFormat& f, const Grid& grid) {
            { WriterFactory<FileFormat>::make(f, grid) } -> std::derived_from<GridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid, typename Comm>
    static constexpr bool has_parallel_factory
        = is_complete<WriterFactory<FileFormat>>
        and requires(const FileFormat& f, const Grid& grid, const Comm& comm) {
            { WriterFactory<FileFormat>::make(f, grid, comm) } -> std::derived_from<GridWriter<Grid>>;
        };

}  // namespace Detail
#endif  // DOXYGEN


template<Concepts::Grid Grid>
class Writer {
 public:
    template<typename FileFormat>
        requires(Detail::has_sequential_factory<FileFormat, Grid>)
    Writer(const FileFormat& f, const Grid& grid)
    : Writer(WriterFactory<FileFormat>::make(f, grid))
    {}

    template<typename FileFormat, Concepts::Communicator Comm>
        requires(Detail::has_parallel_factory<FileFormat, Grid, Comm>)
    Writer(const FileFormat& f, const Grid& grid, const Comm& comm)
    : Writer(WriterFactory<FileFormat>::make(f, grid, comm))
    {}

    template<std::derived_from<GridWriter<Grid>> W> requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _writer(std::make_unique<W>(std::move(writer)))
    {}

    std::string write(const std::string& filename) const {
        return _writer->write(filename);
    }

    template<typename Field>
    void set_meta_data(const std::string& name, Field&& field) {
        _writer->set_meta_data(name, std::forward<Field>(field));
    }

    template<typename Field>
    void set_point_field(const std::string& name, Field&& field) {
        _writer->set_point_field(name, std::forward<Field>(field));
    }

    template<typename Field>
    void set_cell_field(const std::string& name, Field&& field) {
        _writer->set_cell_field(name, std::forward<Field>(field));
    }

    FieldPtr remove_meta_data(const std::string& name) {
        return _writer->remove_meta_data(name);
    }

    FieldPtr remove_point_field(const std::string& name) {
        return _writer->remove_point_field(name);
    }

    FieldPtr remove_cell_field(const std::string& name) {
        return _writer->remove_cell_field(name);
    }

    void clear() {
        _writer->clear();
    }

    template<typename Writer>
    void copy_fields(Writer& w) const {
        _writer->copy_fields(w);
    }

    const std::optional<WriterOptions>& writer_options() const {
        return _writer->writer_options();
    }

    const Grid& grid() const {
        return _writer->grid();
    }

    friend decltype(auto) point_fields(const Writer& w) {
        return point_fields(*w._writer);
    }

    friend decltype(auto) cell_fields(const Writer& w) {
        return cell_fields(*w._writer);
    }

    friend decltype(auto) meta_data_fields(const Writer& w) {
        return meta_data_fields(*w._writer);
    }

 private:
    std::unique_ptr<GridWriter<Grid>> _writer;
};


// template<Concepts::Grid Grid>
// class TimeSeriesWriter {
//  public:
//     template<std::derived_from<TimeSeriesGridWriter<Grid>> W> requires(!std::is_lvalue_reference_v<W>)
//     explicit TimeSeriesWriter(W&& writer)
//     : _writer(std::make_unique<W>(std::move(writer)))
//     {}

//     template<typename... Args>
//     void set_meta_data(Args&&... args) {
//         _writer->set_meta_data(std::forward<Args>(args)...);
//     }

//     template<typename... Args>
//     void set_point_field(Args&&... args) {
//         _writer->set_point_field(std::forward<Args>(args)...);
//     }

//     template<typename... Args>
//     void set_cell_field(Args&&... args) {
//         _writer->set_cell_field(std::forward<Args>(args)...);
//     }

//     std::string write(double t) {
//         return _writer->write(t);
//     }

//  private:
//     std::unique_ptr<TimeSeriesGridWriter<Grid>> _writer;
// };

}  // namespace GridFormat

#endif  // GRIDFORMAT_WRITER_HPP_
