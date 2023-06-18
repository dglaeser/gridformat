// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_TEST_TESTING_HPP_
#define GRIDFORMAT_TEST_TESTING_HPP_

#include <boost/ut.hpp>

namespace GridFormat::Testing {

using boost::ut::expect;
using boost::ut::throws;
using boost::ut::eq;
using boost::ut::operator""_test;

namespace Literals = boost::ut::literals;

}  // namespace GridFormat::Testing

#endif  // GRIDFORMAT_TEST_TESTING_HPP_