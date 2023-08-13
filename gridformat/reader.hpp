// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup API
 * \brief A generic reader providing access to the readers for all supported formats.
 */
#ifndef GRIDFORMAT_READER_HPP_
#define GRIDFORMAT_READER_HPP_

#include <array>
#include <vector>
#include <memory>
#include <utility>
#include <concepts>
#include <functional>
#include <optional>
#include <algorithm>
#include <iterator>
#include <string>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/parallel/concepts.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/reader.hpp>

namespace GridFormat {

namespace FileFormat { struct Any; }

template<typename FileFormat>
struct ReaderFactory;

// Forward declaration, implementation in main API header.
template<Concepts::Communicator C = NullCommunicator>
class AnyReaderFactory {
 public:
    AnyReaderFactory() requires(std::same_as<C, NullCommunicator>) = default;
    explicit AnyReaderFactory(const C& comm) : _comm{comm} {}
    std::unique_ptr<GridReader> make_for(const std::string& filename) const;
 private:
    C _comm;
};

#ifndef DOXYGEN
namespace ReaderDetail {

    template<typename FileFormat>
    concept SequentiallyConstructible
        = is_complete<ReaderFactory<FileFormat>>
        and requires(const FileFormat& f) {
            { ReaderFactory<FileFormat>::make(f) } -> std::derived_from<GridReader>;
        };

    template<typename FileFormat, typename Communicator>
    concept ParallelConstructible
        = is_complete<ReaderFactory<FileFormat>>
        and Concepts::Communicator<Communicator>
        and requires(const FileFormat& f, const Communicator& comm) {
            { ReaderFactory<FileFormat>::make(f, comm) } -> std::derived_from<GridReader>;
        };

    template<typename FieldNameStorage>
    void copy_field_names(const GridReader& reader, FieldNameStorage& names) {
        std::ranges::copy(cell_field_names(reader), std::back_inserter(names.cell_fields));
        std::ranges::copy(point_field_names(reader), std::back_inserter(names.point_fields));
        std::ranges::copy(meta_data_field_names(reader), std::back_inserter(names.meta_data_fields));
    }

    using ReaderFactoryFunctor = std::function<std::unique_ptr<GridReader>(const std::string&)>;

    template<Concepts::Communicator C = NullCommunicator>
    ReaderFactoryFunctor default_reader_factory(const C& c = {}) {
        if (!std::same_as<C, NullCommunicator>)
            return [fac = AnyReaderFactory<C>{c}] (const std::string& f) { return fac.make_for(f); };
        return [fac = AnyReaderFactory<>{}] (const std::string& f) { return fac.make_for(f); };
    }

}  // namespace ReaderDetail
#endif  // DOXYGEN

/*!
 * \ingroup API
 * \brief Interface to the readers for all supported file formats.
 * \details Typically you would construct this class with one of the predefined
 *          file format instances. For example, with the .vtu file format:
 *          \code{.cpp}
 *            GridFormat::Reader reader{GridFormat::vtu};
 *          \endcode
 * \note Unless you pass in a custom ReaderFactoryFunctor, this class requires the
 *       AnyReaderFactory to be defined. Thus, this header cannot be included
 *       without the general API header gridformat.hpp which defines all formats.
 */
class Reader : public GridReader {
 public:
    using ReaderFactoryFunctor = ReaderDetail::ReaderFactoryFunctor;

    Reader(ReaderFactoryFunctor f = ReaderDetail::default_reader_factory())
    : _reader_factory{f}
    {}

    explicit Reader(const FileFormat::Any&, ReaderFactoryFunctor f = ReaderDetail::default_reader_factory())
    : Reader(f)
    {}

    template<Concepts::Communicator C>
    explicit Reader(const FileFormat::Any&, const C& c)
    : Reader(ReaderDetail::default_reader_factory(c))
    {}

    template<ReaderDetail::SequentiallyConstructible FileFormat>
    explicit Reader(const FileFormat& f)
    : _reader{_make_unique(ReaderFactory<FileFormat>::make(f))}
    {}

    template<typename FileFormat,
             Concepts::Communicator Communicator>
        requires(ReaderDetail::ParallelConstructible<FileFormat, Communicator>)
    explicit Reader(const FileFormat& f, const Communicator& c)
    : _reader{_make_unique(ReaderFactory<FileFormat>::make(f, c))}
    {}

 private:
    template<typename ReaderImpl>
    auto _make_unique(ReaderImpl&& reader) const {
        return std::make_unique<std::remove_cvref_t<ReaderImpl>>(std::move(reader));
    }

    std::string _name() const override {
        if (_reader_is_set())
            return _access_reader().name();
        return "undefined";
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& names) override {
        if (_reader_factory)
            _reader = (*_reader_factory)(filename);
        _access_reader().close();
        _access_reader().open(filename);
        ReaderDetail::copy_field_names(_access_reader(), names);
    };

    void _close() override {
        _access_reader().close();
    }

    std::size_t _number_of_cells() const override {
        return _access_reader().number_of_cells();
    }

    std::size_t _number_of_points() const override {
        return _access_reader().number_of_points();
    }

    std::size_t _number_of_pieces() const override {
        return _access_reader().number_of_pieces();
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _access_reader().cell_field(name);
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _access_reader().point_field(name);
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _access_reader().meta_data_field(name);
    }

    void _visit_cells(const CellVisitor& v) const override {
        _access_reader().visit_cells(v);
    }

    FieldPtr _points() const override {
        return _access_reader().points();
    }

    typename GridReader::PieceLocation _location() const override {
        return _access_reader().location();
    }

    std::vector<double> _ordinates(unsigned int dir) const override {
        return _access_reader().ordinates(dir);
    }

    std::array<double, 3> _spacing() const override {
        return _access_reader().spacing();
    }

    std::array<double, 3> _origin() const override {
        return _access_reader().origin();
    }

    std::array<double, 3> _basis_vector(unsigned int dir) const override {
        return _access_reader().basis_vector(dir);
    }

    bool _is_sequence() const override {
        return _access_reader().is_sequence();
    }

    std::size_t _number_of_steps() const override {
        return _access_reader().number_of_steps();
    }

    double _time_at_step(std::size_t step) const override {
        return _access_reader().time_at_step(step);
    }

    void _set_step(std::size_t step, typename GridReader::FieldNames& field_names) override {
        _access_reader().set_step(step);
        field_names.clear();
        ReaderDetail::copy_field_names(_access_reader(), field_names);
    }

    const GridReader& _access_reader() const {
        _check_reader_access();
        return *_reader;
    }

    GridReader& _access_reader() {
        _check_reader_access();
        return *_reader;
    }

    void _check_reader_access() const {
        if (!_reader_is_set())
            throw InvalidState("No file has been read");
    }

    bool _reader_is_set() const {
        return static_cast<bool>(_reader);
    }

    std::unique_ptr<GridReader> _reader;
    std::optional<ReaderFactoryFunctor> _reader_factory;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_WRITER_HPP_
