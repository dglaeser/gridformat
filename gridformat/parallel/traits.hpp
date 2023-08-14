// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Parallel
 * \brief Traits for parallel communication.
 */
#ifndef GRIDFORMAT_PARALLEL_TRAITS_HPP_
#define GRIDFORMAT_PARALLEL_TRAITS_HPP_

#include <array>
#include <ranges>
#include <vector>
#include <type_traits>
#include <algorithm>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat::ParallelTraits {

//! \addtogroup Parallel
//! \{

//! Metafunction to obtain the number of processes from a communicator via a static `int get(const Communicator&)`
template<typename Communicator>
struct Size;

//! Metafunction to obtain the rank of a process from a communicator via a static `int get(const Communicator&)`
template<typename Communicator>
struct Rank;

//! Metafunction to obtain a barrier for all processes to reach before continuation via a static function `int get(const Communicator&)`
template<typename Communicator>
struct Barrier;

//! Metafunction to compute the maximum for a value over all processes via a static function `T get(const Communicator&, const T& values, int root_rank = 0)`
template<typename Communicator>
struct Max;

//! Metafunction to compute the minimum for a value over all processes via a static function `T get(const Communicator&, const T& values, int root_rank = 0)`
template<typename Communicator>
struct Min;

//! Metafunction to compute the sum over values on all processes via a static function `T get(const Communicator&, const T& values, int root_rank = 0)`
template<typename Communicator>
struct Sum;

//! Metafunction to broadcast values from the root to all other processes via a static function `T get(const Communicator&, const T& values, int root_rank = 0)`
template<typename Communicator>
struct BroadCast;

//! Metafunction to gather values from all processes via a static function `std::vector<T> get(const Communicator&, const T& values, int root_rank = 0)`
//! Only the root process will receive the result
template<typename Communicator>
struct Gather;

//! Metafunction to scatter values to all processes via a static function `std::vector<T> get(const Communicator&, const T& values, int root_rank = 0)`
//! Only the root process will receive the result
template<typename Communicator>
struct Scatter;

//! \} group Parallel

}  // namespace GridFormat::ParallelTraits

namespace GridFormat {

struct NullCommunicator {};

namespace ParallelTraits {

template<>
struct Size<NullCommunicator> {
    static constexpr int get(const NullCommunicator&) { return 1; }
};

template<>
struct Rank<NullCommunicator> {
    static constexpr int get(const NullCommunicator&) { return 0; }
};

template<>
struct Barrier<NullCommunicator> {
    static constexpr int get(const NullCommunicator&) { return 0; }
};

template<>
struct Max<NullCommunicator> {
    template<typename T>
    static constexpr const T& get(const NullCommunicator&, const T& data, [[maybe_unused]] int root_rank = 0) {
        return data;
    }
};

template<>
struct Min<NullCommunicator> {
    template<typename T>
    static constexpr const T& get(const NullCommunicator&, const T& data, [[maybe_unused]] int root_rank = 0) {
        return data;
    }
};

template<>
struct Sum<NullCommunicator> {
    template<typename T>
    static constexpr const T& get(const NullCommunicator&, const T& data, [[maybe_unused]] int root_rank = 0) {
        return data;
    }
};

template<>
struct BroadCast<NullCommunicator> {
    template<typename T>
    static constexpr const T& get(const NullCommunicator&, const T& data, [[maybe_unused]] int root_rank = 0) {
        return data;
    }
};

template<>
struct Gather<NullCommunicator> {
    template<Concepts::Scalar T>
    static constexpr std::vector<T> get(const NullCommunicator&, const T& value, [[maybe_unused]] int root_rank = 0) {
        return {value};
    }

    template<Concepts::Scalar T>
    static constexpr const std::vector<T>& get(const NullCommunicator&, const std::vector<T>& vec, [[maybe_unused]] int root_rank = 0) {
        return vec;
    }

    template<std::ranges::range R>
    static constexpr auto get(const NullCommunicator&, const R& r, [[maybe_unused]] int root_rank = 0) {
        std::vector<std::ranges::range_value_t<R>> result(Ranges::size(r));
        std::ranges::copy(r, result.begin());
        return result;
    }
};

template<>
struct Scatter<NullCommunicator> {
 public:
    template<typename T>
    static constexpr const auto& get(const NullCommunicator&, const std::vector<T>& vec, [[maybe_unused]] int root_rank = 0) {
        return vec;
    }

    template<std::ranges::contiguous_range R>
        requires(!Concepts::StaticallySizedRange<R>)
    static constexpr auto get(const NullCommunicator&, const R& r, [[maybe_unused]] int root_rank = 0) {
        std::vector<std::ranges::range_value_t<R>> result;
        result.resize(Ranges::size(r));
        std::ranges::copy(r, result.begin());
        return result;
    }

    template<std::ranges::contiguous_range R>
        requires(Concepts::StaticallySizedRange<R>)
    static constexpr auto get(const NullCommunicator&, const R& r, [[maybe_unused]] int root_rank = 0) {
        std::array<std::ranges::range_value_t<R>, static_size<R>> result;
        std::ranges::copy(r, result.begin());
        return result;
    }
};

}  // namespace ParallelTraits

}  // namespace GridFormat

#if GRIDFORMAT_HAVE_MPI

#include <mpi.h>

#include <gridformat/common/exceptions.hpp>

namespace GridFormat::ParallelTraits {

#ifndef DOXYGEN
namespace MPIDetail {

template<typename T>
decltype(auto) get_data_type() {
    if constexpr (std::is_same_v<T, char>)
        return MPI_CHAR;
    else if constexpr (std::is_same_v<T, signed short int>)
        return MPI_SHORT;
    else if constexpr (std::is_same_v<T, signed int>)
        return MPI_INT;
    else if constexpr (std::is_same_v<T, signed long int>)
        return MPI_LONG;
    else if constexpr (std::is_same_v<T, unsigned char>)
        return MPI_UNSIGNED_CHAR;
    else if constexpr (std::is_same_v<T, unsigned short int>)
        return MPI_UNSIGNED_SHORT;
    else if constexpr (std::is_same_v<T, unsigned int>)
        return MPI_UNSIGNED;
    else if constexpr (std::is_same_v<T, unsigned long int>)
        return MPI_UNSIGNED_LONG;
    else if constexpr (std::is_same_v<T, float>)
        return MPI_FLOAT;
    else if constexpr (std::is_same_v<T, double>)
        return MPI_DOUBLE;
    else if constexpr (std::is_same_v<T, long double>)
        return MPI_LONG_DOUBLE;
    else
        throw TypeError("Cannot deduce mpi type from given type");
}

template<typename T>
void reduce(const T* in, T* out, MPI_Comm comm, MPI_Op operation, int num_values, int root_rank) {
    MPI_Reduce(in, out, num_values, MPIDetail::get_data_type<T>(), operation, root_rank, comm);
}

}  // namespace MPIDetail
#endif  // DOXYGEN

template<>
struct Size<MPI_Comm> {
    static int get(MPI_Comm comm) {
        int s;
        MPI_Comm_size(comm, &s);
        return s;
    }
};

template<>
struct Rank<MPI_Comm> {
    static int get(MPI_Comm comm) {
        int r;
        MPI_Comm_rank(comm, &r);
        return r;
    }
};

template<>
struct Barrier<MPI_Comm> {
    static int get(MPI_Comm comm) {
        return MPI_Barrier(comm);
    }
};

template<>
struct Max<MPI_Comm> {
    template<Concepts::Scalar T>
    static T get(MPI_Comm comm, const T& value, int root_rank = 0) {
        static constexpr int num_values = 1;
        T result;
        MPIDetail::reduce(&value, &result, comm, MPI_MAX, num_values, root_rank);
        return result;
    }

    template<Concepts::StaticallySizedMDRange<1> R> requires(std::ranges::contiguous_range<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        static constexpr int num_values = static_size<R>;
        std::array<std::ranges::range_value_t<R>, num_values> result;
        MPIDetail::reduce(std::ranges::cdata(values), result.data(), comm, MPI_MAX, num_values, root_rank);
        return result;
    }
};

template<>
struct Min<MPI_Comm> {
    template<Concepts::Scalar T>
    static T get(MPI_Comm comm, const T& value, int root_rank = 0) {
        static constexpr int num_values = 1;
        T result;
        MPIDetail::reduce(&value, &result, comm, MPI_MIN, num_values, root_rank);
        return result;
    }

    template<Concepts::StaticallySizedMDRange<1> R> requires(std::ranges::contiguous_range<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        static constexpr int num_values = static_size<R>;
        std::array<std::ranges::range_value_t<R>, num_values> result;
        MPIDetail::reduce(std::ranges::cdata(values), result.data(), comm, MPI_MIN, num_values, root_rank);
        return result;
    }
};

template<>
struct Sum<MPI_Comm> {
    template<Concepts::Scalar T>
    static T get(MPI_Comm comm, const T& value, int root_rank = 0) {
        static constexpr int num_values = 1;
        T result;
        MPIDetail::reduce(&value, &result, comm, MPI_SUM, num_values, root_rank);
        return result;
    }

    template<Concepts::StaticallySizedMDRange<1> R> requires(std::ranges::contiguous_range<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        static constexpr int num_values = static_size<R>;
        std::array<std::ranges::range_value_t<R>, num_values> result;
        MPIDetail::reduce(std::ranges::cdata(values), result.data(), comm, MPI_SUM, num_values, root_rank);
        return result;
    }
};

template<>
struct BroadCast<MPI_Comm> {
    template<Concepts::Scalar T>
    static T get(MPI_Comm comm, const T& value, int root_rank = 0) {
        static constexpr int num_values = 1;
        T result = value;
        MPI_Bcast(
            &result,
            num_values,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }

    template<std::ranges::contiguous_range R> requires(!Concepts::StaticallySizedRange<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        using T = std::ranges::range_value_t<R>;
        const auto num_values = BroadCast<MPI_Comm>::get(comm, std::ranges::size(values), root_rank);
        std::vector<T> result(num_values);
        std::ranges::copy(values, result.begin());
        MPI_Bcast(
            result.data(),
            num_values,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }

    template<std::ranges::contiguous_range R> requires(Concepts::StaticallySizedRange<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        using T = std::ranges::range_value_t<R>;
        static constexpr auto num_values = static_size<R>;
        std::array<T, num_values> result;
        std::ranges::copy(values, result.begin());
        MPI_Bcast(
            result.data(),
            num_values,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }
};

template<>
struct Gather<MPI_Comm> {
    template<Concepts::Scalar T>
    static auto get(MPI_Comm comm, const T& value, int root_rank = 0) {
        static constexpr int num_values = 1;
        const int this_rank = Rank<MPI_Comm>::get(comm);

        std::vector<T> result;
        result.resize(Size<MPI_Comm>::get(comm), T{0});

        MPI_Gather(
            &value,
            num_values,
            MPIDetail::get_data_type<T>(),
            (this_rank == root_rank ? result.data() : NULL),
            num_values,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }

    template<Concepts::StaticallySizedMDRange<1> R> requires(std::ranges::contiguous_range<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        using T = std::ranges::range_value_t<R>;
        static constexpr int num_values = static_size<R>;

        const int this_rank = Rank<MPI_Comm>::get(comm);
        std::vector<T> result(Size<MPI_Comm>::get(comm)*num_values, T{0});
        MPI_Gather(
            std::ranges::cdata(values),
            num_values,
            MPIDetail::get_data_type<T>(),
            (this_rank == root_rank ? result.data() : NULL),
            num_values,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }
};

template<>
struct Scatter<MPI_Comm> {
    template<std::ranges::contiguous_range R> requires(
        !Concepts::StaticallySizedRange<R> and
        std::ranges::sized_range<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        using T = std::ranges::range_value_t<R>;
        const int num_values = static_cast<int>(BroadCast<MPI_Comm>::get(comm, std::ranges::size(values), root_rank));
        const int size = Size<MPI_Comm>::get(comm);
        if (num_values%size != 0)
            throw SizeError("Cannot scatter data with unequal chunks per process");

        std::vector<T> result(num_values/size);
        MPI_Scatter(
            std::ranges::cdata(values),
            num_values/size,
            MPIDetail::get_data_type<T>(),
            result.data(),
            num_values/size,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }

    template<std::ranges::contiguous_range R> requires(Concepts::StaticallySizedRange<R>)
    static auto get(MPI_Comm comm, const R& values, int root_rank = 0) {
        using T = std::ranges::range_value_t<R>;
        const int num_values = static_cast<int>(std::ranges::size(values));
        const int size = Size<MPI_Comm>::get(comm);

        if (num_values%size != 0)
            throw SizeError("Cannot scatter data with unequal chunks per process");

        std::array<T, static_size<R>> result(num_values/size);
        MPI_Scatter(
            std::ranges::cdata(values),
            num_values/size,
            MPIDetail::get_data_type<T>(),
            result.data(),
            num_values/size,
            MPIDetail::get_data_type<T>(),
            root_rank,
            comm
        );
        return result;
    }
};

}  // namespace GridFormat::ParallelTraits

#endif  // GRIDFORMAT_HAVE_MPI

#endif  // GRIDFORMAT_PARALLEL_TRAITS_HPP_
