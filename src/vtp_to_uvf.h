#pragma once
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <string>

vtkSmartPointer<vtkPolyData> parse_vtp_file(const char* vtp_path);
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir);
