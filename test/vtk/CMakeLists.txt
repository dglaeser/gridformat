# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

gridformat_add_regression_test(test_vti_writer test_vti_writer.cpp "vti_*")
gridformat_add_regression_test(test_vtr_writer test_vtr_writer.cpp "vtr_*")
gridformat_add_regression_test(test_vtu_writer test_vtu_writer.cpp "vtu_*")
gridformat_add_regression_test(test_pvd_writer test_pvd_writer.cpp "pvd_time_series*.pvd")
gridformat_add_regression_test(test_vtp_writer test_vtp_writer.cpp "vtp_*")
gridformat_add_parallel_regression_test(test_pvtp_writer test_pvtp_writer.cpp 2 "pvtp_*.pvtp")
gridformat_add_parallel_regression_test(test_pvtu_writer test_pvtu_writer.cpp 2 "pvtu_*.pvtu")
gridformat_add_parallel_regression_test(test_pvti_writer test_pvti_writer.cpp 2 "pvti_*.pvti")
gridformat_add_parallel_regression_test(test_pvtr_writer test_pvtr_writer.cpp 2 "pvtr_*.pvtr")
gridformat_add_parallel_regression_test(test_parallel_pvd_writer test_parallel_pvd_writer.cpp 2 "pvd_parallel_*.pvd")
