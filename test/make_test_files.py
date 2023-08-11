# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

"""Script to prepare test files for the test suite, to be executed upon configure"""

import sys
from os import makedirs
from os.path import abspath, join, dirname

from test_function import TestFunction
try:
    # remove us from the path because there is the test folder 'vtk'
    sys.path.remove(dirname(abspath(__file__)))
    import vtk
    _HAVE_VTK = True
except ImportError:
    _HAVE_VTK = False



def _write_vtu_files(base_filename: str) -> None:
    assert _HAVE_VTK
    grid = _make_unstructured_grid()
    writer = vtk.vtkXMLUnstructuredGridWriter()
    writer.SetInputData(grid)
    _write_xml_files(writer, base_filename, ".vtu")


def _write_vtp_files(base_filename: str) -> None:
    assert _HAVE_VTK
    grid = _make_poly_data_grid()
    writer = vtk.vtkXMLPolyDataWriter()
    writer.SetInputData(grid)
    _write_xml_files(writer, base_filename, ".vtp")


def _write_vts_files(base_filename: str) -> None:
    assert _HAVE_VTK
    grid = _make_structured_grid()
    writer = vtk.vtkXMLStructuredGridWriter()
    writer.SetInputData(grid)
    _write_xml_files(writer, base_filename, ".vts")


def _write_vtr_files(base_filename: str) -> None:
    assert _HAVE_VTK
    grid = _make_rectilinear_grid()
    writer = vtk.vtkXMLRectilinearGridWriter()
    writer.SetInputData(grid)
    _write_xml_files(writer, base_filename, ".vtr")


def _write_vti_files(base_filename: str) -> None:
    assert _HAVE_VTK
    grid = _make_image_grid()
    writer = vtk.vtkXMLImageDataWriter()
    writer.SetInputData(grid)
    _write_xml_files(writer, base_filename, ".vti")


def _make_image_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    grid = vtk.vtkImageData()
    grid.SetExtent(0, n, 0, n, 0, 0)
    grid.SetSpacing(dx, dx, dx)
    _add_fields(grid)
    return grid


def _make_rectilinear_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    grid = vtk.vtkRectilinearGrid()
    grid.SetExtent(0, n, 0, n, 0, 0)
    ordinates = vtk.vtkDoubleArray()
    ordinates.SetNumberOfTuples(n + 1)
    ordinates.SetNumberOfComponents(1)
    for i in range(n + 1):
        ordinates.SetTuple1(i, dx*i)
    grid.SetXCoordinates(ordinates)
    grid.SetYCoordinates(ordinates)
    _add_fields(grid)
    return grid


def _make_structured_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    grid = vtk.vtkStructuredGrid()
    grid.SetExtent(0, n, 0, n, 0, 0)
    grid.SetPoints(_make_points(n, dx))
    _add_fields(grid)
    return grid


def _make_unstructured_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(_make_points(n, dx))
    grid.SetCells(vtk.VTK_QUAD, _make_cells(n))
    _add_fields(grid)
    return grid


def _make_poly_data_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    grid = vtk.vtkPolyData()
    grid.SetPoints(_make_points(n, dx))
    grid.SetPolys(_make_cells(n))
    _add_fields(grid)
    return grid


def _add_fields(grid):
    pscalar = vtk.vtkFloatArray()
    pscalar.SetNumberOfTuples(grid.GetNumberOfPoints())
    pscalar.SetNumberOfComponents(1)
    pscalar.SetName("pscalar")

    cscalar = vtk.vtkFloatArray()
    cscalar.SetNumberOfTuples(grid.GetNumberOfCells())
    cscalar.SetNumberOfComponents(1)
    cscalar.SetName("cscalar")

    for pidx in range(grid.GetNumberOfPoints()):
        test_value = TestFunction()(grid.GetPoint(pidx))
        pscalar.SetTuple1(pidx, test_value)

    for cidx in range(grid.GetNumberOfCells()):
        pids = vtk.vtkIdList()
        grid.GetCellPoints(cidx, pids)
        center = [0., 0., 0.]
        for corner in range(pids.GetNumberOfIds()):
            corner_id = pids.GetId(corner)
            corner_pos = grid.GetPoint(corner_id)
            center = [center[i] + corner_pos[i] for i in range(3)]
        center = [center[i]/pids.GetNumberOfIds() for i in range(3)]
        cscalar.SetTuple1(cidx, TestFunction()(center))

    grid.GetPointData().AddArray(pscalar)
    grid.GetCellData().AddArray(cscalar)
    return grid


def _make_points(n: int, dx: float):
    points = vtk.vtkPoints()
    for y in range(n+1):
        for x in range(n+1):
            position = [float(x)*dx, float(y)*dx, 0.0]
            points.InsertNextPoint(position)
    return points


def _make_cells(n: int):
    cells = vtk.vtkCellArray()
    for y in range(n):
        for x in range(n):
            p0 = y*(n+1) + x
            quad = vtk.vtkQuad()
            quad.GetPointIds().SetId(0, p0)
            quad.GetPointIds().SetId(1, p0 + 1)
            quad.GetPointIds().SetId(2, p0 + n + 2)
            quad.GetPointIds().SetId(3, p0 + n + 1)
            cells.InsertNextCell(quad)
    return cells


def _write_xml_files(writer, base_filename: str, ext: str) -> None:
    writer.SetCompressorTypeToNone()
    writer.EncodeAppendedDataOff()
    writer.SetByteOrderToBigEndian()
    writer.SetFileName(f"{base_filename}_raw_appended_big_endian{ext}")
    writer.Write()

    writer.SetByteOrderToLittleEndian()
    writer.SetFileName(f"{base_filename}_raw_appended_little_endian{ext}")
    writer.Write()

    writer.SetCompressorTypeToZLib()
    writer.SetFileName(f"{base_filename}_raw_appended_little_endian_zlib{ext}")
    writer.Write()

    writer.SetCompressorTypeToLZMA()
    writer.EncodeAppendedDataOn()
    writer.SetFileName(f"{base_filename}_base64_appended_little_endian_lzma{ext}")
    writer.Write()

    writer.SetCompressorTypeToLZ4()
    writer.SetDataModeToBinary()
    writer.SetFileName(f"{base_filename}_base64_inlined_little_endian_lz4{ext}")
    writer.Write()

    writer.SetDataModeToAscii()
    writer.SetFileName(f"{base_filename}_ascii_inlined{ext}")
    writer.Write()


if __name__ == "__main__":
    if not _HAVE_VTK:
        print("Skipping vtk test file generation because vtk package was not found")
        sys.exit(0)

    test_data_path = join(join(abspath(dirname(__file__)), "vtk"), "test_data")
    makedirs(test_data_path, exist_ok=True)

    print("Writing test vtu files")
    _write_vtu_files(join(test_data_path, "vtu_test_file_2d_in_2d"))
    print("Writing test vtp files")
    _write_vtp_files(join(test_data_path, "vtp_test_file_2d_in_2d"))
    print("Writing test vts files")
    _write_vts_files(join(test_data_path, "vts_test_file_2d_in_2d"))
    print("Writing test vtr files")
    _write_vtr_files(join(test_data_path, "vtr_test_file_2d_in_2d"))
    print("Writing test vti files")
    _write_vti_files(join(test_data_path, "vti_test_file_2d_in_2d"))
