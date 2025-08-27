#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkTriangle.h>
#include <cassert>
#include <iostream>
#include "vtp_to_uvf.h"

// Forward declare internal classify function via a local prototype (would normally expose or refactor)
namespace {
// We cannot include private static function from cpp; instead we create datasets that drive generate_uvf and then read manifest
bool load_manifest_kind(const std::string& dir, std::string& kind) {
    std::ifstream ifs(dir + "/manifest.json");
    if(!ifs) return false;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    // Very naive search for geomKind":"<value>"
    auto pos = content.find("\"geomKind\":\"");
    if(pos==std::string::npos) return false;
    pos += 12; // length of "geomKind":"
    auto end = content.find('"', pos);
    if(end==std::string::npos) return false;
    kind = content.substr(pos, end-pos);
    return true;
}
}

static bool test_streamline() {
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    // Create simple polyline (0,0,0)-(1,0,0)-(2,0,0)
    vtkIdType id0 = points->InsertNextPoint(0,0,0);
    vtkIdType id1 = points->InsertNextPoint(1,0,0);
    vtkIdType id2 = points->InsertNextPoint(2,0,0);
    vtkIdType pts[3] = {id0,id1,id2};
    lines->InsertNextCell(3, pts);
    poly->SetPoints(points);
    poly->SetLines(lines);
    std::string outDir = "test_out_stream";
    system((std::string("rm -rf ")+outDir).c_str());
    bool ok = generate_uvf(poly, outDir.c_str());
    if(!ok) return false;
    std::string kind; if(!load_manifest_kind(outDir, kind)) return false; 
    return kind=="streamline"; 
}

static bool test_slice() {
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    // Create a single quad (two triangles) very thin in Z (z=0 plane)
    vtkIdType id0 = points->InsertNextPoint(0,0,0);
    vtkIdType id1 = points->InsertNextPoint(1,0,0);
    vtkIdType id2 = points->InsertNextPoint(1,1,0);
    vtkIdType id3 = points->InsertNextPoint(0,1,0);
    // Two triangles (0,1,2) and (0,2,3)
    vtkIdType tri1[4] = {3,id0,id1,id2}; // legacy style (npts, ids...)? Actually we will push triangles explicitly
    vtkIdType tri2[4] = {3,id0,id2,id3};
    polys->InsertNextCell(3); polys->InsertCellPoint(id0); polys->InsertCellPoint(id1); polys->InsertCellPoint(id2);
    polys->InsertNextCell(3); polys->InsertCellPoint(id0); polys->InsertCellPoint(id2); polys->InsertCellPoint(id3);
    poly->SetPoints(points);
    poly->SetPolys(polys);
    std::string outDir = "test_out_slice";
    system((std::string("rm -rf ")+outDir).c_str());
    bool ok = generate_uvf(poly, outDir.c_str());
    if(!ok) return false;
    std::string kind; if(!load_manifest_kind(outDir, kind)) return false; 
    return kind=="slice"; 
}

static bool test_surface_default() {
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    // Simple tetra base (two triangles not planar test difference on z)
    vtkIdType id0 = points->InsertNextPoint(0,0,0);
    vtkIdType id1 = points->InsertNextPoint(1,0,0);
    vtkIdType id2 = points->InsertNextPoint(0,1,0.2); // slight z to avoid planar threshold
    polys->InsertNextCell(3); polys->InsertCellPoint(id0); polys->InsertCellPoint(id1); polys->InsertCellPoint(id2);
    poly->SetPoints(points);
    poly->SetPolys(polys);
    std::string outDir = "test_out_surface";
    system((std::string("rm -rf ")+outDir).c_str());
    bool ok = generate_uvf(poly, outDir.c_str());
    if(!ok) return false;
    std::string kind; if(!load_manifest_kind(outDir, kind)) return false; 
    return kind=="surface"; 
}

int main() {
    bool a = test_streamline();
    bool b = test_slice();
    bool c = test_surface_default();
    if(!(a&&b&&c)) {
        std::cerr << "Tests failed: " << a << b << c << std::endl;
        return 1;
    }
    std::cout << "All uvf tests passed" << std::endl;
    return 0;
}
