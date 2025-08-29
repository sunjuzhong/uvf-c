# Test Data Directory

This directory contains test data files used by the test suite.

## File Structure

```
data/
├── slice_sample.vtp      # Sample slice geometry
├── line_sample.vtp       # Sample streamline geometry
├── surface_sample.vtk    # Sample surface geometry
└── README.md            # This file
```

## Test Data Requirements

### For test_file_inputs.cpp
The file input tests expect the following test files:
- `slice_sample.vtp`: VTP file containing slice geometry (planar)
- `line_sample.vtp`: VTP file containing line/streamline geometry
- `surface_sample.vtk`: VTK file containing surface geometry

### Creating Test Data

If test data files are missing, you can create them using VTK tools or generate them programmatically:

#### Example: Creating a simple VTP file
```python
import vtk

# Create a simple surface
points = vtk.vtkPoints()
points.InsertNextPoint(0, 0, 0)
points.InsertNextPoint(1, 0, 0)
points.InsertNextPoint(0, 1, 0)

triangle = vtk.vtkTriangle()
triangle.GetPointIds().SetId(0, 0)
triangle.GetPointIds().SetId(1, 1)
triangle.GetPointIds().SetId(2, 2)

triangles = vtk.vtkCellArray()
triangles.InsertNextCell(triangle)

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)
polydata.SetPolys(triangles)

writer = vtk.vtkXMLPolyDataWriter()
writer.SetFileName("surface_sample.vtp")
writer.SetInputData(polydata)
writer.Write()
```

## Notes

- Test files should be small and focused on specific geometry types
- Files should contain representative data arrays when testing metadata extraction
- Keep file sizes minimal to ensure fast test execution
