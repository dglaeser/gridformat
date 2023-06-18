// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper struct to create an overload set from callables.
 */
#ifndef GRIDFORMAT_COMMON_CALL_OPERATOR_OVERLOAD_SET_HPP_
#define GRIDFORMAT_COMMON_CALL_OPERATOR_OVERLOAD_SET_HPP_

namespace GridFormat {

/*!
 * \file
 * \ingroup Common
 * \brief Helper struct to create an overload set from callables.
 */
template<typename... Ts>
struct Overload : Ts... { using Ts::operator()...; };
template<typename... Ts> Overload(Ts...) -> Overload<Ts...>;

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_OVERLOAD_SET_HPP_
