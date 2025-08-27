#include "vtk_structured_parser.h"
#include "vtp_to_uvf.h"
#include "id_utils.h"
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::pair;

// Multi-file UVF generator based on your diagram structure
bool generate_multi_file_uvf(
    const vector<string>& vtk_files,
    const vector<string>& file_labels,
    const char* uvf_dir
) {
    if (vtk_files.empty() || vtk_files.size() != file_labels.size()) {
        return false;
    }
    
    string out_dir = string(uvf_dir);
    string resources_dir = out_dir + "/resources/uvf";
    
    // Create directory structure
    make_dirs(out_dir);
    make_dirs(out_dir + "/resources");
    make_dirs(resources_dir);
    
    // Categorize files based on labels and contents
    map<string, vector<pair<string, string>>> groups; // group_name -> [(file_path, label)]
    
    for (size_t i = 0; i < vtk_files.size(); ++i) {
        const string& file_path = vtk_files[i];
        const string& label = file_labels[i];
        string lower_label = label;
        std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(), ::tolower);
        
        string group_name = "surfaces"; // default
        
        if (lower_label.find("slice") != string::npos || 
            lower_label.find("plane") != string::npos ||
            lower_label.find("xy") != string::npos ||
            lower_label.find("xz") != string::npos ||
            lower_label.find("yz") != string::npos) {
            group_name = "slices";
        }
        else if (lower_label.find("surface") != string::npos ||
                 lower_label.find("boundary") != string::npos ||
                 lower_label.find("internal") != string::npos) {
            group_name = "surfaces";
        }
        else if (lower_label.find("iso") != string::npos ||
                 lower_label.find("value") != string::npos ||
                 lower_label.find("level") != string::npos) {
            group_name = "isosurfaces";
        }
        else if (lower_label.find("stream") != string::npos ||
                 lower_label.find("line") != string::npos ||
                 lower_label.find("seed") != string::npos) {
            group_name = "streamlines";
        }
        
        groups[group_name].push_back({file_path, label});
    }
    
    // Process each file and generate binary data
    map<string, UVFOffsets> all_offsets;
    
    for (const auto& group : groups) {
        for (const auto& file_info : group.second) {
            const string& file_path = file_info.first;
            const string& label = file_info.second;
            
            // Load VTK file
            auto poly = parse_vtp_file(file_path.c_str());
            if (!poly) {
                std::cerr << "Failed to load: " << file_path << std::endl;
                continue;
            }
            
            // Extract geometry data
            vector<float> vertices;
            vector<uint32_t> indices;
            map<string, vector<float>> scalar_data;
            
            if (!extract_geometry_data(poly, vertices, indices, scalar_data)) {
                std::cerr << "Failed to extract data from: " << file_path << std::endl;
                continue;
            }
            
            // Write binary file
            string bin_path = resources_dir + "/" + label + ".bin";
            UVFOffsets offsets;
            if (!write_binary_data(vertices, indices, scalar_data, bin_path, offsets)) {
                std::cerr << "Failed to write binary data for: " << label << std::endl;
                continue;
            }
            
            all_offsets[label] = offsets;
        }
    }
    
    // Generate manifest according to your diagram structure
    std::ostringstream manifest_ss;
    manifest_ss << "[";
    
    bool first = true;
    
    // 1. Root GeometryGroup
    if (!first) manifest_ss << ",";
    first = false;
    manifest_ss << "{";
    manifest_ss << "\"id\":\"root_group\",";
    manifest_ss << "\"type\":\"GeometryGroup\",";
    manifest_ss << "\"properties\":{\"type\":0,\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]},";
    manifest_ss << "\"attributions\":{\"members\":[";
    bool first_member = true;
    for (const auto& group : groups) {
        if (!first_member) manifest_ss << ",";
        first_member = false;
        manifest_ss << "\"" << group.first << "\"";
    }
    manifest_ss << "]}";
    manifest_ss << "}";
    
    // 2. Group-level GeometryGroups
    for (const auto& group : groups) {
        if (!first) manifest_ss << ",";
        first = false;
        manifest_ss << "{";
        manifest_ss << "\"id\":\"" << group.first << "\",";
        manifest_ss << "\"type\":\"GeometryGroup\",";
        manifest_ss << "\"properties\":{\"type\":0},";
        manifest_ss << "\"attributions\":{\"members\":[";
        
        bool first_sub_member = true;
        for (const auto& file_info : group.second) {
            if (!first_sub_member) manifest_ss << ",";
            first_sub_member = false;
            manifest_ss << "\"" << file_info.second << "\"";
        }
        manifest_ss << "]}";
        manifest_ss << "}";
        
        // 3. SolidGeometry for each file in the group
        for (const auto& file_info : group.second) {
            const string& label = file_info.second;
            
            if (!first) manifest_ss << ",";
            first = false;
            manifest_ss << "{";
            manifest_ss << "\"id\":\"" << label << "\",";
            manifest_ss << "\"type\":\"SolidGeometry\",";
            manifest_ss << "\"properties\":{},";
            manifest_ss << "\"attributions\":{";
            manifest_ss << "\"edges\":[],";
            manifest_ss << "\"vertices\":[],";
            manifest_ss << "\"faces\":[\"" << label << "\"]";
            manifest_ss << "}";
            
            // Resources
            auto offset_it = all_offsets.find(label);
            if (offset_it != all_offsets.end()) {
                manifest_ss << ",\"resources\":{\"buffers\":{";
                manifest_ss << "\"path\":\"resources/uvf/" << label << ".bin\",";
                manifest_ss << "\"sections\":[";
                
                bool first_section = true;
                for (const auto& field : offset_it->second.fields) {
                    if (!first_section) manifest_ss << ",";
                    first_section = false;
                    manifest_ss << "{";
                    manifest_ss << "\"dType\":\"" << field.second.dType << "\",";
                    manifest_ss << "\"dimension\":" << field.second.dimension << ",";
                    manifest_ss << "\"length\":" << field.second.length << ",";
                    manifest_ss << "\"name\":\"" << field.first << "\",";
                    manifest_ss << "\"offset\":" << field.second.offset;
                    manifest_ss << "}";
                }
                
                manifest_ss << "],\"type\":\"buffers\"}}";
            }
            
            manifest_ss << "}";
            
            // 4. Face for each SolidGeometry
            if (!first) manifest_ss << ",";
            first = false;
            manifest_ss << "{";
            manifest_ss << "\"id\":\"" << label << "\",";
            manifest_ss << "\"type\":\"Face\",";
            manifest_ss << "\"properties\":{";
            manifest_ss << "\"alpha\":1.0,";
            manifest_ss << "\"color\":16777215";
            
            // Buffer locations
            auto offset_it2 = all_offsets.find(label);
            if (offset_it2 != all_offsets.end()) {
                auto indices_it = offset_it2->second.fields.find("indices");
                if (indices_it != offset_it2->second.fields.end()) {
                    size_t num_triangles = indices_it->second.length / sizeof(uint32_t) / 3;
                    manifest_ss << ",\"bufferLocations\":{\"indices\":[{";
                    manifest_ss << "\"bufNum\":0,";
                    manifest_ss << "\"startIndex\":0,";
                    manifest_ss << "\"endIndex\":" << num_triangles;
                    manifest_ss << "}]}";
                }
            }
            
            manifest_ss << "},";
            manifest_ss << "\"attributions\":{\"packedParentId\":\"" << label << "\"}";
            manifest_ss << "}";
        }
    }
    
    manifest_ss << "]";
    
    // Write manifest
    string manifest_path = out_dir + "/manifest.json";
    std::ofstream ofs(manifest_path);
    if (!ofs) return false;
    
    ofs << manifest_ss.str();
    ofs.close();
    
    return true;
}

// Enhanced CLI interface for multi-file processing
bool process_directory_structure(const char* input_dir, const char* uvf_dir) {
    vector<string> vtk_files;
    vector<string> file_labels;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
            if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".vtk" || ext == ".vtp") {
                    vtk_files.push_back(entry.path().string());
                    string stem = entry.path().stem().string();
                    string cleaned_stem = clean_id(stem);
                    file_labels.push_back(cleaned_stem);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
        return false;
    }
    
    if (vtk_files.empty()) {
        std::cerr << "No VTK files found in directory: " << input_dir << std::endl;
        return false;
    }
    
    std::cout << "Found " << vtk_files.size() << " VTK files:" << std::endl;
    for (size_t i = 0; i < vtk_files.size(); ++i) {
        std::cout << "  " << file_labels[i] << " -> " << vtk_files[i] << std::endl;
    }
    
    return generate_multi_file_uvf(vtk_files, file_labels, uvf_dir);
}
