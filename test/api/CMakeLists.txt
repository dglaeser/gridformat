# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

gridformat_add_regression_test(test_generic_writer test_generic_writer.cpp "generic_writer_*")
gridformat_add_regression_test(test_generic_time_series_writer test_generic_time_series_writer.cpp "generic_time_series_*pvd")
gridformat_add_parallel_regression_test(test_generic_parallel_writer test_generic_parallel_writer.cpp 2 "generic_parallel_writer_*")
gridformat_add_parallel_regression_test(test_generic_parallel_time_series_writer test_generic_parallel_time_series_writer.cpp 2 "generic_parallel_time_series_*pvd")
