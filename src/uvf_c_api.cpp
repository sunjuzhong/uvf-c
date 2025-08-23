#include "vtp_to_uvf.h"
#include <string>
#include <vector>
#include <memory>
#include <vtkXMLPolyDataReader.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <mutex>

// Simple state for last operation
namespace {
    std::string g_last_error;
    int g_last_point_count = 0;
    int g_last_triangle_count = 0;
    std::mutex g_mutex;
    void set_error(const std::string& e){ std::lock_guard<std::mutex> lk(g_mutex); g_last_error = e; }
    void set_stats(int pts, int tris){ std::lock_guard<std::mutex> lk(g_mutex); g_last_point_count=pts; g_last_triangle_count=tris; }
}

extern "C" {
// Basic existence check
int parse_vtp(const char* vtp_path) {
    auto poly = parse_vtp_file(vtp_path);
    if(!poly){ set_error("Parse failed"); return 0; }
    set_stats(poly->GetNumberOfPoints(), poly->GetNumberOfPolys());
    return 1;
}

// Generate UVF directory from VTP file path -> output dir
int generate_uvf(const char* vtp_path, const char* uvf_dir) {
    auto poly = parse_vtp_file(vtp_path);
    if (!poly){ set_error("Parse failed"); return 0; }
    bool ok = generate_uvf(poly, uvf_dir);
    if(!ok){ set_error("UVF generation failed"); return 0; }
    // Count triangles (converted polys fan triangulation approximated via GetNumberOfPolys for now)
    set_stats(poly->GetNumberOfPoints(), static_cast<int>(poly->GetNumberOfPolys()));
    return 1;
}

// Extended API: return last error (pointer valid until next call)
const char* uvf_get_last_error(){ std::lock_guard<std::mutex> lk(g_mutex); return g_last_error.c_str(); }
int uvf_get_last_point_count(){ std::lock_guard<std::mutex> lk(g_mutex); return g_last_point_count; }
int uvf_get_last_triangle_count(){ std::lock_guard<std::mutex> lk(g_mutex); return g_last_triangle_count; }
}
