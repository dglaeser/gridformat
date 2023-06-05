// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_WRITER_HPP_
#define GRIDFORMAT_WRITER_HPP_

#include <string>
#include <memory>
#include <utility>
#include <functional>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
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
        and Concepts::Grid<Grid>
        and requires(const FileFormat& f, const Grid& grid) {
            { WriterFactory<FileFormat>::make(f, grid) } -> std::derived_from<GridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid>
    static constexpr bool has_sequential_time_series_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and requires(const FileFormat& f, const Grid& grid) {
            { WriterFactory<FileFormat>::make(f, grid, std::string{}) } -> std::derived_from<TimeSeriesGridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid, typename Comm>
    static constexpr bool has_parallel_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and Concepts::Communicator<Comm>
        and requires(const FileFormat& f, const Grid& grid, const Comm& comm) {
            { WriterFactory<FileFormat>::make(f, grid, comm) } -> std::derived_from<GridWriter<Grid>>;
        };

    template<typename FileFormat, typename Grid, typename Comm>
    static constexpr bool has_parallel_time_series_factory
        = is_complete<WriterFactory<FileFormat>>
        and Concepts::Grid<Grid>
        and Concepts::Communicator<Comm>
        and requires(const FileFormat& f, const Grid& grid, const Comm& comm) {
            { WriterFactory<FileFormat>::make(f, grid, comm, std::string{}) } -> std::derived_from<TimeSeriesGridWriter<Grid>>;
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

    template<typename FileFormat>
        requires(Detail::has_sequential_time_series_factory<FileFormat, Grid>)
    Writer(const FileFormat& f, const Grid& grid, const std::string& base_filename)
    : Writer(WriterFactory<FileFormat>::make(f, grid, base_filename))
    {}

    template<typename FileFormat, Concepts::Communicator Comm>
        requires(Detail::has_parallel_factory<FileFormat, Grid, Comm>)
    Writer(const FileFormat& f, const Grid& grid, const Comm& comm)
    : Writer(WriterFactory<FileFormat>::make(f, grid, comm))
    {}

    template<typename FileFormat, Concepts::Communicator Comm>
        requires(Detail::has_parallel_time_series_factory<FileFormat, Grid, Comm>)
    Writer(const FileFormat& f, const Grid& grid, const Comm& comm, const std::string& base_filename)
    : Writer(WriterFactory<FileFormat>::make(f, grid, comm, base_filename))
    {}

    template<std::derived_from<GridWriter<Grid>> W>
        requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _writer(std::make_unique<W>(std::move(writer)))
    {}

    template<std::derived_from<TimeSeriesGridWriter<Grid>> W>
        requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _time_series_writer(std::make_unique<W>(std::move(writer)))
    {}

    std::string write(const std::string& filename) const {
        if (!_writer)
            throw InvalidState(
                "Writer was constructed as a time series writer. Only write(Scalar) can be used."
            );
        return _writer->write(filename);
    }

    template<Concepts::Scalar T>
    std::string write(const T& time_value) const {
        if (!_time_series_writer)
            throw InvalidState(
                "Writer was not constructed as a time series writer. Only write(std::string) can be used."
            );
        return _time_series_writer->write(time_value);
    }

    template<typename Field>
    void set_meta_data(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_meta_data(name, std::forward<Field>(field));
        });
    }

    template<typename Field>
    void set_point_field(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_point_field(name, std::forward<Field>(field));
        });
    }

    template<typename Field, typename T>
    void set_point_field(const std::string& name, Field&& field, const Precision<T>& prec) {
        _visit_writer([&] (auto& writer) {
            writer.set_point_field(name, std::forward<Field>(field), prec);
        });
    }

    template<typename Field>
    void set_cell_field(const std::string& name, Field&& field) {
        _visit_writer([&] (auto& writer) {
            writer.set_cell_field(name, std::forward<Field>(field));
        });
    }

    template<typename Field, typename T>
    void set_cell_field(const std::string& name, Field&& field, const Precision<T>& prec) {
        _visit_writer([&] (auto& writer) {
            writer.set_cell_field(name, std::forward<Field>(field), prec);
        });
    }

    FieldPtr remove_meta_data(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_meta_data(name);
        });
    }

    FieldPtr remove_point_field(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_point_field(name);
        });
    }

    FieldPtr remove_cell_field(const std::string& name) {
        return _visit_writer([&] (auto& writer) {
            return writer.remove_cell_field(name);
        });
    }

    void clear() {
        _visit_writer([&] (auto& writer) {
            writer.clear();
        });
    }

    template<typename Writer>
    void copy_fields(Writer& out) const {
        _visit_writer([&] (const auto& writer) {
            writer.copy_fields(out);
        });
    }

    const std::optional<WriterOptions>& writer_options() const {
        return _visit_writer([&] (const auto& writer) -> const std::optional<WriterOptions>& {
            return writer.writer_options();
        });
    }

    const Grid& grid() const {
        return _visit_writer([&] (const auto& writer) -> const Grid& {
            return writer.grid();
        });
    }

    friend decltype(auto) point_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return point_fields(writer);
        });
    }

    friend decltype(auto) cell_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return cell_fields(writer);
        });
    }

    friend decltype(auto) meta_data_fields(const Writer& w) {
        return w._visit_writer([&] (const auto& writer) {
            return meta_data_fields(writer);
        });
    }

 private:
    template<typename Visitor>
    decltype(auto) _visit_writer(const Visitor& visitor) {
        if (_writer)
            return std::invoke(visitor, *_writer);
        else {
            if (!_time_series_writer)
                throw InvalidState("No writer set");
            return std::invoke(visitor, *_time_series_writer);
        }
    }

    template<typename Visitor>
    decltype(auto) _visit_writer(const Visitor& visitor) const {
        if (_writer)
            return std::invoke(visitor, *_writer);
        else {
            if (!_time_series_writer)
                throw InvalidState("No writer set");
            return std::invoke(visitor, *_time_series_writer);
        }
    }

    std::unique_ptr<GridWriter<Grid>> _writer{nullptr};
    std::unique_ptr<TimeSeriesGridWriter<Grid>> _time_series_writer{nullptr};
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_WRITER_HPP_
