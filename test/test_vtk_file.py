# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

from os.path import splitext
from argparse import ArgumentParser
from dataclasses import dataclass
from typing import Callable, Tuple, List
from math import sin, cos, isclose
from xml.etree import ElementTree
from sys import exit

class _TestFunction:
    def __init__(self, scaling: float = 1.0) -> None:
        self._scaling = scaling

    def __call__(self, point: list[float]) -> float:
        result = 10.0*sin(point[0])
        if len(point) > 1:
            result *= cos(point[1])
        if len(point) > 2:
            result *= point[2] + 1.0
        return result*self._scaling


class VTKErrorObserver:
   def __init__(self):
       self.__ErrorOccurred = False
       self.__ErrorMessage = None
       self.CallDataType = 'string0'

   def __call__(self, obj, event, message):
       self.__ErrorOccurred = True
       self.__ErrorMessage = message

   def ErrorOccurred(self):
       occ = self.__ErrorOccurred
       self.__ErrorOccurred = False
       return occ

   def ErrorMessage(self):
       return self.__ErrorMessage


@dataclass
class _TimeStep:
    filename: str
    time: float


def _add_points(p0: tuple, p1: tuple) -> tuple:
        assert len(p0) == len(p1)
        return tuple(p0[i] + p1[i] for i in range(len(p0)))


def _scale_point(p: tuple, factor: float) -> tuple:
    return tuple(p[i]*factor for i in range(len(p)))


def _restrict_to_space_dim(point: tuple, space_dim: int) -> tuple:
    if space_dim == 1:
        return tuple([point[0]])
    if space_dim == 2:
        return tuple([point[0], point[1]])
    if space_dim == 3:
        return tuple([point[0], point[1], point[2]])


def _get_grid_and_space_dimension(filename: str) -> Tuple[int, int]:
    dim, space_dim = filename.split("_in_")
    dim = dim[-2:-1]
    space_dim = space_dim[0]
    return int(dim), int(space_dim)


def _check_vtk_file(vtk_reader,
                    space_dim,
                    reference_function: Callable[[list], float]) -> None:
    rel_tol = 1e-5
    abs_tol = 1e-3

    output = vtk_reader.GetOutput()
    points = output.GetPoints()

    def _compute_cell_center(cell_id: int, ) -> tuple:
        from vtk import vtkIdList
        ids = vtkIdList()
        output.GetCellPoints(cell_id, ids)
        result = (0., 0., 0.)
        for i in range(ids.GetNumberOfIds()):
            result = _add_points(result, points.GetPoint(ids.GetId(i)))
        return _scale_point(result, 1.0/float(ids.GetNumberOfIds()))

    def _compare_data_array(arr, position_call_back):
        for i in range(arr.GetNumberOfTuples()):
            point = position_call_back(i)
            value = arr.GetTuple(i)
            reference = reference_function(_restrict_to_space_dim(point, space_dim))
            for comp in range(arr.GetNumberOfComponents()):
                if comp/3 < space_dim and comp%3 < space_dim:
                    assert isclose(reference, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)
                else:
                    assert isclose(0.0, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)

    point_data = output.GetPointData()
    for i in range(point_data.GetNumberOfArrays()):
        name = point_data.GetArrayName(i)
        arr = point_data.GetArray(i)
        print(f"Comparing point array '{name}'")
        for i in range(arr.GetNumberOfTuples()):
            _compare_data_array(arr, lambda i: points.GetPoint(i))

    cell_data = output.GetCellData()
    for i in range(cell_data.GetNumberOfArrays()):
        name = cell_data.GetArrayName(i)
        arr = cell_data.GetArray(i)
        print(f"Comparing cell array '{name}'")
        for i in range(arr.GetNumberOfTuples()):
            _compare_data_array(arr, lambda i: _compute_cell_center(i))


def _read_pvd_pieces(filename: str) -> List[_TimeStep]:
    tree = ElementTree.parse(filename)
    root = tree.getroot()
    return [
        _TimeStep(
            filename=data_set.attrib["file"],
            time=data_set.attrib["timestep"]
        ) for data_set in root.find("Collection")
    ]


def _read_pvtu_pieces(filename: str) -> List[str]:
    tree = ElementTree.parse(filename)
    root = tree.getroot()
    return [
        piece.attrib["Source"]
        for piece in root.find("PUnstructuredGrid").findall("Piece")
    ]


def _test_vtk(filename: str, reference_function: Callable[[list], float]):
    try:
        from vtk import vtkXMLUnstructuredGridReader, vtkXMLPolyDataReader
    except ImportError:
        print("VTK not found")
        exit(255)

    e = VTKErrorObserver()
    ext = splitext(filename)[1]
    if ext == ".vtu":
        reader = vtkXMLUnstructuredGridReader()
        reader.AddObserver("ErrorEvent", e)
    elif ext == ".vtp":
        reader = vtkXMLPolyDataReader()
        reader.AddObserver("ErrorEvent", e)
    else:
        raise NotImplementedError("Unsupported vtk extension")
    reader.SetFileName(filename)
    reader.Update()
    if e.ErrorOccurred():
        raise IOError(f"Error reading VTK file '{filename}': {e.ErrorMessage()}")

    _, space_dim = _get_grid_and_space_dimension(filename)
    _check_vtk_file(reader, space_dim, reference_function)


def test(filename: str) -> None:
    ext = splitext(filename)[1]
    if ext in [".vtu", ".vtp"]:
        print(f"Comparing file '{filename}'")
        _test_vtk(filename, _TestFunction())
    elif ext == ".pvd":
        for timestep in _read_pvd_pieces(filename):
            print(f"Comparing timestep '{timestep.time}' in file {timestep.filename}")
            if splitext(timestep.filename)[1].startswith(".p"):
                for piece in _read_pvtu_pieces(timestep.filename):
                    print(f" -- Comparing piece '{piece}'")
                    _test_vtk(piece, _TestFunction(float(timestep.time)))
            else:
                _test_vtk(timestep.filename, _TestFunction(float(timestep.time)))
    elif ext == ".pvtu":
        print(f"Comparing pvtu file {filename}")
        for piece in _read_pvtu_pieces(filename):
            print(f"Comparing piece '{piece}'")
            _test_vtk(piece, _TestFunction())
    else:
        print(f"No reader for files with extension {ext}")
        exit(255)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-p", "--filepath", required=True)
    args = vars(parser.parse_args())
    test(args["filepath"])
