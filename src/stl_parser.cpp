#include "stl_parser.h"
#include <vtkSTLReader.h>
#include <string>
#include <algorithm>

vtkSmartPointer<vtkPolyData> parse_stl_file(const char* path) {
    if (!path) return nullptr;

    auto reader = vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(path);
    reader->Update();

    auto output = reader->GetOutput();
    if (!output || output->GetNumberOfPoints() == 0) {
        return nullptr;
    }

    return output;
}

