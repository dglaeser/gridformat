# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

from math import sin, cos

class TestFunction:
    """Callable that represents the analytical function we use on grid files"""
    def __init__(self, scaling: float = 1.0) -> None:
        self._scaling = scaling

    def __call__(self, point: list[float]) -> float:
        result = 10.0*sin(point[0])
        if len(point) > 1:
            result *= cos(point[1])
        if len(point) > 2:
            result *= point[2] + 1.0
        return result*self._scaling

    def set_time(self, value: float) -> None:
        self._scaling = value
