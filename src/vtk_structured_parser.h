#pragma once

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;
using std::map;

// Forward declarations
struct UVFOffsets;

// Structure for VTK data classification
namespace VTKStructured {
    struct DataGroup {
        string group_name;
        string group_type; 
        vector<string> data_names;
        map<string, vtkSmartPointer<vtkPolyData>> poly_data;
    };
}

// Main API functions
bool generate_structured_uvf(vtkPolyData* poly, const char* uvf_dir);

// Helper functions  
bool extract_geometry_data(
    vtkPolyData* poly,
    vector<float>& vertices,
    vector<uint32_t>& indices,
    map<string, vector<float>>& scalar_data
);

// Directory creation helper
void make_dirs(const std::string& path);

// Binary data writing function (from existing code)
bool write_binary_data(
    const vector<float>& vertices, 
    const vector<uint32_t>& indices, 
    const map<string, vector<float>>& scalar_data, 
    const string& bin_path, 
    UVFOffsets& offsets
);
