# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

gridformat_add_regression_test(test_vtu_writer test_vtu_writer.cpp "vtu_*")
gridformat_add_regression_test(test_pvd_writer test_pvd_writer.cpp "*.pvd")
gridformat_add_regression_test(test_vtp_writer test_vtp_writer.cpp "vtp_*")
gridformat_add_parallel_regression_test(test_pvtu_writer test_pvtu_writer.cpp 2 "*.pvtu")
gridformat_add_parallel_regression_test(test_parallel_pvd_writer test_parallel_pvd_writer.cpp 2 "pvd_parallel_*.pvd")
