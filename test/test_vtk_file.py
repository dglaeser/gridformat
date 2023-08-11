# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

import sys
from os.path import splitext, dirname, abspath
from argparse import ArgumentParser
from dataclasses import dataclass
from typing import Callable, Tuple, List
from math import sqrt, isclose
from xml.etree import ElementTree
from numpy import array, ndarray, sum as np_sum
from sys import exit

from test_function import TestFunction
try:
    # remove us from the path because there is the test folder 'vtk'
    sys.path.remove(dirname(abspath(__file__)))
    import vtk
    HAVE_VTK = True
except ImportError:
    HAVE_VTK = False



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
                    points,
                    space_dim,
                    reference_function: Callable[[list], float],
                    skip_metadata: bool,
                    is_discontinuous: bool = False) -> None:
    rel_tol = 1e-5
    abs_tol = 1e-3

    output = vtk_reader.GetOutput()
    if not skip_metadata:
        field_data = output.GetFieldData()
        expected_field_data = ["literal", "string", "numbers"]
        for i in range(field_data.GetNumberOfArrays()):
            name = field_data.GetAbstractArray(i).GetName()
            if name in expected_field_data:
                expected_field_data.remove(name)
        if expected_field_data:
            raise RuntimeError(f"Did not find the following metadata: {expected_field_data}")
        else:
            print("Found all expected field data")
        if field_data.GetArray("TimeValue") is not None:
            assert field_data.GetArray("TimeValue").GetNumberOfTuples() == 1
            assert field_data.GetArray("TimeValue").GetNumberOfComponents() == 1
            time_value = field_data.GetArray("TimeValue").GetValue(0)
            print(f"Scaling with time = {time_value}")
            reference_function.set_time(time_value)

    # precompute cell centers
    num_cells = output.GetNumberOfCells()
    points = array(points)
    cell_centers = ndarray(shape=(num_cells, 3))
    for cell_id in range(num_cells):
        ids = vtk.vtkIdList()
        output.GetCellPoints(cell_id, ids)
        corner_indices = [ids.GetId(_i) for _i in range(ids.GetNumberOfIds())]
        cell_centers[cell_id] = np_sum(points[corner_indices], axis=0)
        cell_centers[cell_id] /= float(ids.GetNumberOfIds())

    def _compare_data_array(arr, position_call_back):
        for i in range(arr.GetNumberOfTuples()):
            point = position_call_back(i)
            value = arr.GetTuple(i)
            reference = reference_function(_restrict_to_space_dim(point, space_dim))
            ncomps = arr.GetNumberOfComponents()
            vtk_dim = int(sqrt(ncomps)) if ncomps > 3 else ncomps
            for comp in range(arr.GetNumberOfComponents()):
                if comp/vtk_dim < space_dim and comp%vtk_dim < space_dim:
                    assert isclose(reference, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)
                else:
                    assert isclose(0.0, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)

    point_data = output.GetPointData()
    for i in range(point_data.GetNumberOfArrays()):
        name = point_data.GetArrayName(i)
        arr = point_data.GetArray(i)
        print(f"Comparing point array '{name}'")
        if is_discontinuous:
            # each point of the cell should have the embedding cell index as value
            point_id = 0
            for cell_id in range(num_cells):
                ids = vtk.vtkIdList()
                output.GetCellPoints(cell_id, ids)
                for _ in range(ids.GetNumberOfIds()):
                    assert isclose(arr.GetTuple(point_id)[0], float(cell_id))
                    point_id += 1
        else:
            for i in range(arr.GetNumberOfTuples()):
                _compare_data_array(arr, lambda i: points[i])

    cell_data = output.GetCellData()
    for i in range(cell_data.GetNumberOfArrays()):
        name = cell_data.GetArrayName(i)
        arr = cell_data.GetArray(i)
        print(f"Comparing cell array '{name}'")
        for i in range(arr.GetNumberOfTuples()):
            _compare_data_array(arr, lambda i: cell_centers[i])


def _read_pvd_pieces(filename: str) -> List[_TimeStep]:
    tree = ElementTree.parse(filename)
    root = tree.getroot()
    return [
        _TimeStep(
            filename=data_set.attrib["file"],
            time=data_set.attrib["timestep"]
        ) for data_set in root.find("Collection")
    ]


def _test_vtk(filename: str, skip_metadata: bool, reference_function: Callable[[list], float]):
    def _get_points_from_grid(reader):
        points = reader.GetOutput().GetPoints()
        return array([points.GetPoint(i) for i in range(points.GetNumberOfPoints())])

    def _get_rectilinear_points(reader):
        output = reader.GetOutput()
        return array([output.GetPoint(i) for i in range(output.GetNumberOfPoints())])

    e = VTKErrorObserver()
    ext = splitext(filename)[1]
    if ext == ".vtu":
        reader = vtk.vtkXMLUnstructuredGridReader()
        point_collector = _get_points_from_grid
    elif ext == ".pvtu":
        reader = vtk.vtkXMLPUnstructuredGridReader()
        point_collector = _get_points_from_grid
    elif ext == ".vtp":
        reader = vtk.vtkXMLPolyDataReader()
        point_collector = _get_points_from_grid
    elif ext == ".vtr":
        reader = vtk.vtkXMLRectilinearGridReader()
        point_collector = _get_rectilinear_points
    elif ext == ".vts":
        reader = vtk.vtkXMLStructuredGridReader()
        point_collector = _get_points_from_grid
    elif ext == ".pvts":
        reader = vtk.vtkXMLPStructuredGridReader()
        point_collector = _get_points_from_grid
    elif ext == ".pvtr":
        reader = vtk.vtkXMLPRectilinearGridReader()
        point_collector = _get_rectilinear_points
    elif ext == ".pvtp":
        reader = vtk.vtkXMLPPolyDataReader()
        point_collector = _get_points_from_grid
    elif ext == ".vti":
        reader = vtk.vtkXMLImageDataReader()
        point_collector = _get_rectilinear_points
    elif ext == ".pvti":
        reader = vtk.vtkXMLPImageDataReader()
        point_collector = _get_rectilinear_points
    elif ext == ".hdf" and "image" in filename:
        reader = vtk.vtkHDFReader()
        point_collector = _get_rectilinear_points
    elif ext == ".hdf" and "unstructured" in filename:
        reader = vtk.vtkHDFReader()
        point_collector = _get_points_from_grid
    else:
        raise NotImplementedError(f"Could not determine suitable reader {filename}")
    reader.AddObserver("ErrorEvent", e)
    reader.SetFileName(filename)
    reader.Update()
    if e.ErrorOccurred():
        raise IOError(f"Error reading VTK file '{filename}': {e.ErrorMessage()}")

    _, space_dim = _get_grid_and_space_dimension(filename)
    is_discontinuous = "discontinuous" in filename
    _check_vtk_file(reader, point_collector(reader), space_dim, reference_function, skip_metadata, is_discontinuous)


def test(filename: str, skip_metadata: bool = False) -> int | None:
    if not HAVE_VTK:
        return 42

    ext = splitext(filename)[1]
    if ext == ".pvd":
        for timestep in _read_pvd_pieces(filename):
            print(f"Comparing timestep '{timestep.time}' in file {timestep.filename}")
            _test_vtk(timestep.filename, skip_metadata, TestFunction(float(timestep.time)))
    else:
        print(f"Comparing file '{filename}'")
        _test_vtk(filename, skip_metadata, TestFunction())


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-p", "--filepath", required=True)
    parser.add_argument("-sm", "--skip-metadata", required=False, action="store_true")
    args = vars(parser.parse_args())
    ret_code = test(args["filepath"], skip_metadata=(True if args["skip_metadata"] else False))
    if ret_code is not None:
        exit(ret_code)
