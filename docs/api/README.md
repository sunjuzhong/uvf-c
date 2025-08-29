# API Reference

## Core Functions

### `parse_vtp_file`
```cpp
vtkSmartPointer<vtkPolyData> parse_vtp_file(const char* path);
```
Parse VTP (XML) or legacy VTK polydata files into vtkPolyData.

**Parameters:**
- `path`: Path to the VTP/VTK file

**Returns:**
- Smart pointer to vtkPolyData, or null if parsing fails

### `generate_uvf` (Basic)
```cpp
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir);
```
Generate UVF format from vtkPolyData.

**Parameters:**
- `poly`: Input vtkPolyData
- `uvf_dir`: Output directory path

**Returns:**
- `true` if successful, `false` otherwise

### `generate_uvf` (Enhanced)
```cpp
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir, vector<DataArrayInfo>* array_info);
```
Generate UVF format with DataArray information extraction.

**Parameters:**
- `poly`: Input vtkPolyData
- `uvf_dir`: Output directory path
- `array_info`: Pointer to vector for storing DataArray metadata

**Returns:**
- `true` if successful, `false` otherwise

## Data Structures

### `DataArrayInfo`
```cpp
struct DataArrayInfo {
    string name;        // Array name
    int components;     // Number of components
    size_t tuples;      // Number of tuples
    float rangeMin;     // Minimum value
    float rangeMax;     // Maximum value
    string dType;       // Data type
};
```
Contains metadata about VTK data arrays.

### `UVFOffsets`
```cpp
struct UVFOffsets {
    struct Info {
        size_t offset;
        size_t length;
        string dType;
        int dimension;
    };
    map<string, Info> fields;
};
```
Stores binary data offset information for UVF files.
