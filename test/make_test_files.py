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
    _write_xml_files(writer, base_filename)


def _make_unstructured_grid(n: int = 5, dx: float = 0.1):
    assert _HAVE_VTK
    points = vtk.vtkPoints()
    cells = vtk.vtkCellArray()

    pscalar = vtk.vtkFloatArray()
    pscalar.SetNumberOfTuples((n+1)**2)
    pscalar.SetNumberOfComponents(1)
    pscalar.SetName("pscalar")

    cscalar = vtk.vtkFloatArray()
    cscalar.SetNumberOfTuples(n**2)
    cscalar.SetNumberOfComponents(1)
    cscalar.SetName("cscalar")

    for y in range(n+1):
        for x in range(n+1):
            position = [float(x)*dx, float(y)*dx, 0.0]
            test_value = TestFunction()(position)
            points.InsertNextPoint(position)
            pscalar.SetTuple1(y*(n+1) + x, test_value)

    for y in range(n):
        for x in range(n):
            p0 = y*(n+1) + x
            quad = vtk.vtkQuad()
            center = [(float(x)+0.5)*dx, (float(y)+0.5)*dx, 0.0]
            test_value = TestFunction()(center)

            quad.GetPointIds().SetId(0, p0)
            quad.GetPointIds().SetId(1, p0 + 1)
            quad.GetPointIds().SetId(2, p0 + n + 2)
            quad.GetPointIds().SetId(3, p0 + n + 1)
            cells.InsertNextCell(quad)
            cscalar.SetTuple1(y*n + x, test_value)


    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(points)
    grid.SetCells(vtk.VTK_QUAD, cells)
    grid.GetPointData().AddArray(pscalar)
    grid.GetCellData().AddArray(cscalar)
    return grid


def _write_xml_files(writer, base_filename: str) -> None:
    writer.SetCompressorTypeToNone()
    writer.EncodeAppendedDataOff()
    writer.SetByteOrderToBigEndian()
    writer.SetFileName(f"{base_filename}_raw_appended_big_endian.vtu")
    writer.Write()

    writer.SetByteOrderToLittleEndian()
    writer.SetFileName(f"{base_filename}_raw_appended_little_endian.vtu")
    writer.Write()

    writer.SetCompressorTypeToZLib()
    writer.SetFileName(f"{base_filename}_raw_appended_little_endian_zlib.vtu")
    writer.Write()

    writer.SetCompressorTypeToLZMA()
    writer.EncodeAppendedDataOn()
    writer.SetFileName(f"{base_filename}_base64_appended_little_endian_lzma.vtu")
    writer.Write()

    writer.SetCompressorTypeToLZ4()
    writer.SetDataModeToBinary()
    writer.SetFileName(f"{base_filename}_base64_inlined_little_endian_lz4.vtu")
    writer.Write()


if __name__ == "__main__":
    if not _HAVE_VTK:
        print("Skipping vtk test file generation because vtk package was not found")
        sys.exit(0)

    test_data_path = join(join(abspath(dirname(__file__)), "vtk"), "test_data")
    makedirs(test_data_path, exist_ok=True)

    print("Writing test vtu files")
    _write_vtu_files(join(test_data_path, "vtu_test_file"))
