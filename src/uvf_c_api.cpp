#include "vtp_to_uvf.h"
#include "vtk_structured_parser.h"
#include "multi_file_parser.h"
#include <string>
#include <vector>
#include <memory>
#include <vtkXMLPolyDataReader.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <mutex>
#include <filesystem>

// Enhanced state for tracking multiple operations
namespace {
    std::string g_last_error;
    int g_last_point_count = 0;
    int g_last_triangle_count = 0;
    int g_last_file_count = 0;
    int g_last_group_count = 0;
    std::string g_last_operation_type;
    std::mutex g_mutex;
    
    void set_error(const std::string& e){ 
        std::lock_guard<std::mutex> lk(g_mutex); 
        g_last_error = e; 
    }
    
    void set_stats(int pts, int tris, int files = 1, int groups = 1, const std::string& op_type = "basic"){ 
        std::lock_guard<std::mutex> lk(g_mutex); 
        g_last_point_count = pts; 
        g_last_triangle_count = tris;
        g_last_file_count = files;
        g_last_group_count = groups;
        g_last_operation_type = op_type;
    }
}

extern "C" {

// ====== Basic Functions ======

// Basic existence check  
int parse_vtp(const char* vtp_path) {
    auto poly = parse_vtp_file(vtp_path);
    if(!poly){ 
        set_error("Parse failed"); 
        return 0; 
    }
    set_stats(poly->GetNumberOfPoints(), poly->GetNumberOfPolys(), 1, 1, "parse_check");
    return 1;
}

// Generate UVF directory from VTP file path -> output dir (basic mode)
int generate_uvf(const char* vtp_path, const char* uvf_dir) {
    auto poly = parse_vtp_file(vtp_path);
    if (!poly){ 
        set_error("Parse failed"); 
        return 0; 
    }
    bool ok = ::generate_uvf(poly, uvf_dir);
    if(!ok){ 
        set_error("UVF generation failed"); 
        return 0; 
    }
    set_stats(poly->GetNumberOfPoints(), static_cast<int>(poly->GetNumberOfPolys()), 1, 1, "basic_uvf");
    return 1;
}

// ====== Enhanced Functions ======

// Generate UVF using structured parsing (field-based classification)
int generate_uvf_structured(const char* vtp_path, const char* uvf_dir) {
    auto poly = parse_vtp_file(vtp_path);
    if (!poly){ 
        set_error("Parse failed"); 
        return 0; 
    }
    bool ok = generate_structured_uvf(poly, uvf_dir);
    if(!ok){ 
        set_error("Structured UVF generation failed"); 
        return 0; 
    }
    set_stats(poly->GetNumberOfPoints(), static_cast<int>(poly->GetNumberOfPolys()), 1, 2, "structured_uvf");
    return 1;
}

// Generate UVF from directory with multiple VTK files (recommended)
int generate_uvf_directory(const char* input_dir, const char* uvf_dir) {
    try {
        // Count files first
        int file_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".vtk" || ext == ".vtp") {
                    file_count++;
                }
            }
        }
        
        if (file_count == 0) {
            set_error("No VTK files found in directory");
            return 0;
        }
        
        bool ok = process_directory_structure(input_dir, uvf_dir);
        if(!ok){ 
            set_error("Directory UVF generation failed"); 
            return 0; 
        }
        
        // Estimate stats (actual values would need parsing all files)
        set_stats(0, 0, file_count, 2, "directory_multi");
        return 1;
        
    } catch (const std::exception& e) {
        set_error(std::string("Directory processing error: ") + e.what());
        return 0;
    }
}

// ====== Status and Information Functions ======

// Extended API: return last error (pointer valid until next call)
const char* uvf_get_last_error(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_error.c_str(); 
}

int uvf_get_last_point_count(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_point_count; 
}

int uvf_get_last_triangle_count(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_triangle_count; 
}

int uvf_get_last_file_count(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_file_count; 
}

int uvf_get_last_group_count(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_group_count; 
}

const char* uvf_get_last_operation_type(){ 
    std::lock_guard<std::mutex> lk(g_mutex); 
    return g_last_operation_type.c_str(); 
}

// ====== Utility Functions ======

// Check if a path is a directory
int uvf_is_directory(const char* path) {
    try {
    return std::filesystem::is_directory(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

// Count VTK files in a directory
int uvf_count_vtk_files(const char* dir_path) {
    try {
        int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".vtk" || ext == ".vtp") {
                    count++;
                }
            }
        }
        return count;
    } catch (...) {
        return -1;
    }
}

// Get API version
const char* uvf_get_version() {
    return "0.1.1-structured-multi";
}
}
