from os.path import splitext
from argparse import ArgumentParser
from typing import Callable, Tuple
from math import sin, cos, isclose
from sys import exit

class TestFunction:
    def __init__(self, scaling: float = 1.0) -> None:
        self._scaling = scaling

    def __call__(self, point: list[float]) -> float:
        result = 10.0*sin(point[0])
        if len(point) > 1:
            result *= cos(point[1])
        if len(point) > 2:
            result *= point[2] + 1.0
        return result


def get_grid_and_space(filename: str) -> Tuple[int, int]:
    dim, space_dim = filename.split("_in_")
    dim = dim[-2:-1]
    space_dim = space_dim[0]
    return int(dim), int(space_dim)


def check_vtk_file(filename: str,
                   reference_function: Callable[[list], float]) -> None:
    rel_tol = 1e-5
    abs_tol = 1e-3
    _, space_dim = get_grid_and_space(filename)

    try:
        from vtk import vtkXMLUnstructuredGridReader, vtkXMLPolyDataReader, vtkIdList
    except ImportError:
        print("VTK not found")
        exit(255)

    ext = splitext(filename)[1]
    if ext == ".vtu":
        reader = vtkXMLUnstructuredGridReader()
    elif ext == ".vtp":
        reader = vtkXMLPolyDataReader()
    else:
        print(f"Extension {ext} not yet supported")
        exit(255)
    reader.SetFileName(filename)
    reader.Update()
    output = reader.GetOutput()
    points = output.GetPoints()

    def _compute_cell_center(cell_id: int, ) -> tuple:
        ids = vtkIdList()
        output.GetCellPoints(cell_id, ids)
        result = (0., 0., 0.)
        for i in range(ids.GetNumberOfIds()):
            result = _add_points(result, points.GetPoint(ids.GetId(i)))
        return _scale_point(result, 1.0/float(ids.GetNumberOfIds()))

    point_data = output.GetPointData()
    for i in range(point_data.GetNumberOfArrays()):
        name = point_data.GetArrayName(i)
        arr = point_data.GetArray(i)
        print(f"Comparing point array '{name}'")
        for i in range(arr.GetNumberOfTuples()):
            point = points.GetPoint(i)
            value = arr.GetTuple(i)
            reference = reference_function(_restrict_to_space_dim(point, space_dim))
            for comp in range(arr.GetNumberOfComponents()):
                if comp/3 < space_dim and comp%3 < space_dim:
                    assert isclose(reference, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)
                else:
                    assert isclose(0.0, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)

    cell_data = output.GetCellData()
    for i in range(cell_data.GetNumberOfArrays()):
        name = cell_data.GetArrayName(i)
        arr = cell_data.GetArray(i)
        print(f"Comparing cell array '{name}'")
        for i in range(arr.GetNumberOfTuples()):
            point = _compute_cell_center(i)
            value = arr.GetTuple(i)
            reference = reference_function(_restrict_to_space_dim(point, space_dim))
            for comp in range(arr.GetNumberOfComponents()):
                if comp/3 < space_dim and comp%3 < space_dim:
                    assert isclose(reference, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)
                else:
                    assert isclose(0.0, value[comp], rel_tol=rel_tol, abs_tol=abs_tol)


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


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-p", "--filepath", required=True)
    args = vars(parser.parse_args())
    check_vtk_file(args["filepath"], TestFunction())
