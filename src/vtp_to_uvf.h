#pragma once
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <string>

// Parse either .vtp (XML) or legacy .vtk polydata/unstructured grid into vtkPolyData
vtkSmartPointer<vtkPolyData> parse_vtp_file(const char* path);
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir);
