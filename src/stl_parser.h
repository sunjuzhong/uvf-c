#pragma once
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

/**
 * Parse an STL file (ASCII or Binary format) into vtkPolyData
 * @param path Path to the STL file
 * @return vtkPolyData containing the mesh, or nullptr on failure
 */
vtkSmartPointer<vtkPolyData> parse_stl_file(const char* path);

