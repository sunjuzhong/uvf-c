#include "vtk_structured_parser.h"
#include "vtp_to_uvf.h"
#include "id_utils.h"
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

using std::vector;
using std::string;
using std::map;
using std::set;

// Helper function to generate JSON manually
string create_json_array(const vector<string>& items) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << items[i] << "\"";
    }
    oss << "]";
    return oss.str();
}

string create_json_array(const vector<int>& items) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << ",";
        oss << items[i];
    }
    oss << "]";
    return oss.str();
}

string create_transform_matrix() {
    return "[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]";
}

// Structure to represent UVF geometry hierarchy
struct UVFGeometry {
    string id;
    string type; // "GeometryGroup", "SolidGeometry", "Face"
    map<string, string> properties;
    map<string, string> attributions;
    map<string, string> resources;
    vector<string> members; // for GeometryGroup
    vector<string> faces;   // for SolidGeometry
};

// Enhanced VTK data classifier
class VTKDataClassifier {
public:
    struct DataGroup {
        string group_name;
        string group_type; // slices, surfaces, isosurfaces, streamlines
        vector<string> data_names;
        map<string, vtkSmartPointer<vtkPolyData>> poly_data;
    };
    
    static vector<DataGroup> classify_vtk_data(vtkPolyData* poly) {
        vector<DataGroup> groups;
        
        if (!poly || !poly->GetPointData()) return groups;
        
        auto pd = poly->GetPointData();
        set<string> field_names;
        
        // Collect all field names
        for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
            auto arr = pd->GetArray(i);
            if (arr && arr->GetName()) {
                field_names.insert(string(arr->GetName()));
            }
        }
        
        // Classify based on naming patterns and data characteristics
        DataGroup slices_group = {"slices", "slices", {}, {}};
        DataGroup surfaces_group = {"surfaces", "surfaces", {}, {}};
        DataGroup isosurfaces_group = {"isosurfaces", "isosurfaces", {}, {}};
        DataGroup streamlines_group = {"streamlines", "streamlines", {}, {}};
        
        for (const auto& name : field_names) {
            string lower_name = name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            
            if (lower_name.find("slice") != string::npos || 
                lower_name.find("plane") != string::npos ||
                lower_name.find("_xy_") != string::npos ||
                lower_name.find("_xz_") != string::npos ||
                lower_name.find("_yz_") != string::npos) {
                slices_group.data_names.push_back(name);
                slices_group.poly_data[name] = extract_field_geometry(poly, name);
            }
            else if (lower_name.find("surface") != string::npos ||
                     lower_name.find("boundary") != string::npos ||
                     lower_name.find("internal") != string::npos) {
                surfaces_group.data_names.push_back(name);
                surfaces_group.poly_data[name] = extract_field_geometry(poly, name);
            }
            else if (lower_name.find("iso") != string::npos ||
                     lower_name.find("value") != string::npos ||
                     lower_name.find("level") != string::npos) {
                isosurfaces_group.data_names.push_back(name);
                isosurfaces_group.poly_data[name] = extract_field_geometry(poly, name);
            }
            else if (lower_name.find("stream") != string::npos ||
                     lower_name.find("line") != string::npos ||
                     lower_name.find("seed") != string::npos) {
                streamlines_group.data_names.push_back(name);
                streamlines_group.poly_data[name] = extract_field_geometry(poly, name);
            }
        }
        
        // Only add groups that have data
        if (!slices_group.data_names.empty()) groups.push_back(slices_group);
        if (!surfaces_group.data_names.empty()) groups.push_back(surfaces_group);
        if (!isosurfaces_group.data_names.empty()) groups.push_back(isosurfaces_group);
        if (!streamlines_group.data_names.empty()) groups.push_back(streamlines_group);
        
        // If no specific patterns found, create a default group
        if (groups.empty()) {
            DataGroup default_group = {"default", "surfaces", {"main"}, {}};
            default_group.poly_data["main"] = poly;
            groups.push_back(default_group);
        }
        
        return groups;
    }
    
private:
    static vtkSmartPointer<vtkPolyData> extract_field_geometry(vtkPolyData* poly, const string& field_name) {
        // For now, return the whole geometry
        // In a more sophisticated implementation, you might filter based on field values
        auto result = vtkSmartPointer<vtkPolyData>::New();
        result->DeepCopy(poly);
        return result;
    }
};

// Enhanced manifest generator with manual JSON generation
class StructuredManifestGenerator {
public:
    static bool generate_structured_manifest(
        const vector<VTKDataClassifier::DataGroup>& groups,
        const map<string, UVFOffsets>& all_offsets,
        const string& output_dir,
        string& manifest_path
    ) {
        std::ostringstream manifest_ss;
        manifest_ss << "[";
        
        bool first = true;
        
        // 1. Create root GeometryGroup
        if (!first) manifest_ss << ",";
        first = false;
        manifest_ss << create_root_group_json(groups);
        
        // 2. Create sub GeometryGroups for each data type
        for (const auto& group : groups) {
            if (!first) manifest_ss << ",";
            first = false;
            manifest_ss << create_sub_geometry_group_json(group);
            
            // 3. Create SolidGeometry for each data item in the group
            for (const auto& data_name : group.data_names) {
                if (!first) manifest_ss << ",";
                first = false;
                manifest_ss << create_solid_geometry_json(group, data_name, all_offsets);
                
                // 4. Create Face for each solid geometry
                if (!first) manifest_ss << ",";
                first = false;
                manifest_ss << create_face_json(group, data_name, all_offsets);
            }
        }
        
        manifest_ss << "]";
        
        // Write manifest file
        manifest_path = output_dir + "/manifest.json";
        std::ofstream ofs(manifest_path);
        if (!ofs) return false;
        
        ofs << manifest_ss.str();
        ofs.close();
        
        return true;
    }
    
private:
    static string create_root_group_json(const vector<VTKDataClassifier::DataGroup>& groups) {
        std::ostringstream ss;
        ss << "{";
        ss << "\"id\":\"root_group\",";
        ss << "\"type\":\"GeometryGroup\",";
        ss << "\"properties\":{";
        ss << "\"type\":0,";
        ss << "\"transform\":" << create_transform_matrix();
        ss << "},";
        ss << "\"attributions\":{";
        ss << "\"members\":[";
        for (size_t i = 0; i < groups.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << groups[i].group_name << "\"";
        }
        ss << "]";
        ss << "}";
        ss << "}";
        return ss.str();
    }
    
    static string create_sub_geometry_group_json(const VTKDataClassifier::DataGroup& group) {
        std::ostringstream ss;
        ss << "{";
        ss << "\"id\":\"" << group.group_name << "\",";
        ss << "\"type\":\"GeometryGroup\",";
        ss << "\"properties\":{\"type\":0},";
        ss << "\"attributions\":{";
        ss << "\"members\":[";
        for (size_t i = 0; i < group.data_names.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << group.data_names[i] << "\"";
        }
        ss << "]";
        ss << "}";
        ss << "}";
        return ss.str();
    }
    
    static string create_solid_geometry_json(
        const VTKDataClassifier::DataGroup& group,
        const string& data_name,
        const map<string, UVFOffsets>& all_offsets
    ) {
        std::ostringstream ss;
        ss << "{";
        ss << "\"id\":\"" << data_name << "\",";
        ss << "\"type\":\"SolidGeometry\",";
        ss << "\"properties\":{},";
        ss << "\"attributions\":{";
        ss << "\"edges\":[],";
        ss << "\"vertices\":[],";
        ss << "\"faces\":[\"" << data_name << "\"]";
        ss << "}";
        
        // Resources - binary data reference
        auto offset_it = all_offsets.find(data_name);
        if (offset_it != all_offsets.end()) {
            ss << ",\"resources\":{";
            ss << "\"buffers\":{";
            ss << "\"path\":\"resources/uvf/" << data_name << ".bin\",";
            ss << "\"sections\":[";
            
            bool first_section = true;
            for (const auto& field : offset_it->second.fields) {
                if (!first_section) ss << ",";
                first_section = false;
                ss << "{";
                ss << "\"dType\":\"" << field.second.dType << "\",";
                ss << "\"dimension\":" << field.second.dimension << ",";
                ss << "\"length\":" << field.second.length << ",";
                ss << "\"name\":\"" << field.first << "\",";
                ss << "\"offset\":" << field.second.offset;
                ss << "}";
            }
            
            ss << "],";
            ss << "\"type\":\"buffers\"";
            ss << "}";
            ss << "}";
        }
        
        ss << "}";
        return ss.str();
    }
    
    static string create_face_json(
        const VTKDataClassifier::DataGroup& group,
        const string& data_name,
        const map<string, UVFOffsets>& all_offsets
    ) {
        std::ostringstream ss;
        ss << "{";
        ss << "\"id\":\"" << data_name << "\",";
        ss << "\"type\":\"Face\",";
        ss << "\"properties\":{";
        ss << "\"alpha\":1.0,";
        ss << "\"color\":16777215";
        
        // Buffer locations
        auto offset_it = all_offsets.find(data_name);
        if (offset_it != all_offsets.end()) {
            auto indices_it = offset_it->second.fields.find("indices");
            if (indices_it != offset_it->second.fields.end()) {
                size_t num_triangles = indices_it->second.length / sizeof(uint32_t) / 3;
                ss << ",\"bufferLocations\":{";
                ss << "\"indices\":[{";
                ss << "\"bufNum\":0,";
                ss << "\"startIndex\":0,";
                ss << "\"endIndex\":" << num_triangles;
                ss << "}]";
                ss << "}";
            }
        }
        
        ss << "},";
        ss << "\"attributions\":{";
        ss << "\"packedParentId\":\"" << data_name << "_SolidGeometry\"";
        ss << "}";
        ss << "}";
        return ss.str();
    }
};

// Main function to generate structured UVF from VTK
bool generate_structured_uvf(vtkPolyData* poly, const char* uvf_dir) {
    if (!poly) return false;
    
    string out_dir = string(uvf_dir);
    string resources_dir = out_dir + "/resources/uvf";
    
    // Create directory structure
    make_dirs(out_dir);
    make_dirs(out_dir + "/resources");
    make_dirs(resources_dir);
    
    // Classify VTK data into groups
    auto groups = VTKDataClassifier::classify_vtk_data(poly);
    
    // Extract and write binary data for each group
    map<string, UVFOffsets> all_offsets;
    
    for (const auto& group : groups) {
        for (const auto& data_entry : group.poly_data) {
            const string& data_name = data_entry.first;
            auto data_poly = data_entry.second;
            
            // Extract geometry data
            vector<float> vertices;
            vector<uint32_t> indices;
            map<string, vector<float>> scalar_data;
            
            if (!extract_geometry_data(data_poly, vertices, indices, scalar_data)) {
                continue;
            }
            
            // Write binary file
            string bin_path = resources_dir + "/" + data_name + ".bin";
            UVFOffsets offsets;
            if (!write_binary_data(vertices, indices, scalar_data, bin_path, offsets)) {
                continue;
            }
            
            all_offsets[data_name] = offsets;
        }
    }
    
    // Generate structured manifest
    string manifest_path;
    return StructuredManifestGenerator::generate_structured_manifest(
        groups, all_offsets, out_dir, manifest_path
    );
}
