#pragma once
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <string>
#include <vector>
#include <map>

using std::vector;
using std::string;
using std::map;

// DataArray information structure
struct DataArrayInfo {
    string name;        // Array name
    int components;     // Number of components
    size_t tuples;      // Number of tuples
    float rangeMin;     // Minimum value
    float rangeMax;     // Maximum value
    string dType;       // Data type
};

// UVF offset structure for binary data
struct UVFOffsets {
    struct Info {
        size_t offset;
        size_t length;
        string dType;
        int dimension;
    };
    map<string, Info> fields;
};

// Parse either .vtp (XML) or legacy .vtk polydata/unstructured grid into vtkPolyData
vtkSmartPointer<vtkPolyData> parse_vtp_file(const char* path);

// Generate basic UVF format
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir);

// Generate UVF format with DataArray information (enhanced version)
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir, vector<DataArrayInfo>* array_info);

// Extract geometry data from vtkPolyData
bool extract_geometry_data(
    vtkPolyData* polydata, 
    vector<float>& vertices, 
    vector<uint32_t>& indices, 
    map<string, vector<float>>& scalar_data
);

// Write binary data and return offset information
bool write_binary_data(
    const vector<float>& vertices, 
    const vector<uint32_t>& indices, 
    const map<string, vector<float>>& scalar_data, 
    const string& bin_path, 
    UVFOffsets& offsets
);
