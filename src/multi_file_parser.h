#pragma once

#include <vector>
#include <string>

// Multi-file UVF generation based on structured parsing
bool generate_multi_file_uvf(
    const std::vector<std::string>& vtk_files,
    const std::vector<std::string>& file_labels,
    const char* uvf_dir
);

// Process all VTK files in a directory
bool process_directory_structure(const char* input_dir, const char* uvf_dir);
