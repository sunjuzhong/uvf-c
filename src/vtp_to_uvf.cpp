
#include "vtp_to_uvf.h"
#include "vtk_structured_parser.h"
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkGeometryFilter.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <random>
#include <vtkCellData.h>
#include <vtkFieldData.h>
#include <vtkAbstractArray.h>
#include <vtkStringArray.h>

// Face segmentation support (FaceIndex + FaceIdMapping)
struct UVFFaceSegment {
    std::string id;        // face id (mapped name or generated)
    size_t startIndex = 0; // index into global indices array (uint32 element index, inclusive)
    size_t endIndex = 0;   // exclusive end
};

using std::vector;
using std::string;
using std::map;

// Detect extension helper
static std::string file_ext_lower(const char* path){
    std::string s(path?path:"");
    auto pos = s.find_last_of('.');
    if(pos==std::string::npos) return ""; 
    std::string ext = s.substr(pos+1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

// Generate a readable random token (avoid confusing chars 0,O,1,l)
static std::string make_random_token(size_t len=8){
    static const char charset[] = "abcdefghijkmnpqrstuvwxyz23456789"; // 32 chars
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, sizeof(charset)-2);
    std::string out; out.reserve(len);
    for(size_t i=0;i<len;++i) out.push_back(charset[dist(gen)]);
    return out;
}

// Simple wrapper to read either VTP (XML PolyData) or legacy VTK (PolyData or UnstructuredGrid) and return vtkPolyData
vtkSmartPointer<vtkPolyData> parse_vtp_file(const char* path) {
    if(!path) return nullptr;
    std::string ext = file_ext_lower(path);
    vtkSmartPointer<vtkPolyData> output;
    if(ext == "vtp") {
        auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(path);
        if(!reader->CanReadFile(path)) return nullptr;
        reader->Update();
        output = reader->GetOutput();
    } else if(ext == "vtk") {
        // Try legacy polydata reader first
        auto pdReader = vtkSmartPointer<vtkPolyDataReader>::New();
        pdReader->SetFileName(path);
        if(pdReader->IsFilePolyData()) {
            pdReader->Update();
            output = pdReader->GetOutput();
        } else {
            // Try unstructured grid then convert via geometry filter
            auto ugReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
            ugReader->SetFileName(path);
            if(ugReader->IsFileUnstructuredGrid()) {
                ugReader->Update();
                auto geom = vtkSmartPointer<vtkGeometryFilter>::New();
                geom->SetInputData(ugReader->GetOutput());
                geom->Update();
                output = geom->GetOutput();
            }
        }
    } else {
        // Unknown extension; still attempt XML polydata in case
        auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(path);
        if(reader->CanReadFile(path)) {
            reader->Update();
            output = reader->GetOutput();
        }
    }
    if(output && output->GetNumberOfPoints()==0) { // Guard against empty dataset leading to downstream issues
        return nullptr;
    }
    return output;
}

// Parse VTP file, extract vertices, indices, scalar fields
bool read_vtp_data(const char* filename, vector<float>& vertices, vector<uint32_t>& indices, map<string, vector<float>>& scalar_data) {
    auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader->SetFileName(filename);
    reader->Update();
    auto polydata = reader->GetOutput();
    if (!polydata) return false;
    auto pts = polydata->GetPoints();
    if (!pts) return false;
    vtkIdType nPts = pts->GetNumberOfPoints();
    vertices.resize(nPts * 3);
    for (vtkIdType i = 0; i < nPts; ++i) {
        double p[3];
        pts->GetPoint(i, p);
        vertices[i * 3 + 0] = static_cast<float>(p[0]);
        vertices[i * 3 + 1] = static_cast<float>(p[1]);
        vertices[i * 3 + 2] = static_cast<float>(p[2]);
    }
    // Triangulate polys
    auto polys = polydata->GetPolys();
    auto idList = vtkSmartPointer<vtkIdList>::New();
    polys->InitTraversal();
    while (polys->GetNextCell(idList)) {
        if (idList->GetNumberOfIds() < 3) continue;
        for (vtkIdType j = 1; j < idList->GetNumberOfIds() - 1; ++j) {
            indices.push_back(static_cast<uint32_t>(idList->GetId(0)));
            indices.push_back(static_cast<uint32_t>(idList->GetId(j)));
            indices.push_back(static_cast<uint32_t>(idList->GetId(j + 1)));
        }
    }
    // Fallback: if no polys produced indices and there are line cells, encode each segment as degenerate triangle (a,b,b)
    if(indices.empty() && polydata->GetLines() && polydata->GetLines()->GetNumberOfCells()>0){
        auto lines = polydata->GetLines();
        lines->InitTraversal();
        while(lines->GetNextCell(idList)){
            if(idList->GetNumberOfIds()<2) continue;
            for(vtkIdType j=0;j<idList->GetNumberOfIds()-1;++j){
                uint32_t a = static_cast<uint32_t>(idList->GetId(j));
                uint32_t b = static_cast<uint32_t>(idList->GetId(j+1));
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(b); // degenerate triangle to preserve triplet structure
            }
        }
    }
    // Scalar fields
    auto pd = polydata->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        auto arr = pd->GetArray(i);
        if (!arr) continue;
        string name = arr->GetName() ? arr->GetName() : ("field" + std::to_string(i));
        int nComp = arr->GetNumberOfComponents();
        vtkIdType nTuples = arr->GetNumberOfTuples();
        vector<float> data(nTuples * nComp);
        for (vtkIdType t = 0; t < nTuples; ++t) {
            for (int c = 0; c < nComp; ++c) {
                data[t * nComp + c] = static_cast<float>(arr->GetComponent(t, c));
            }
        }
        scalar_data[name] = std::move(data);
    }
    return true;
}

// Write binary data, return offsets info
bool write_binary_data(const vector<float>& vertices, const vector<uint32_t>& indices, const map<string, vector<float>>& scalar_data, const string& bin_path, UVFOffsets& offsets) {
    std::ofstream ofs(bin_path, std::ios::binary);
    if (!ofs) return false;
    size_t current_offset = 0;
    // Indices
    ofs.write(reinterpret_cast<const char*>(indices.data()), indices.size() * sizeof(uint32_t));
    offsets.fields["indices"] = {current_offset, indices.size() * sizeof(uint32_t), "uint32", 1};
    current_offset += indices.size() * sizeof(uint32_t);
    // Vertices
    ofs.write(reinterpret_cast<const char*>(vertices.data()), vertices.size() * sizeof(float));
    offsets.fields["position"] = {current_offset, vertices.size() * sizeof(float), "float32", 3};
    current_offset += vertices.size() * sizeof(float);
    // Scalar fields
    for (const auto& kv : scalar_data) {
        const string& name = kv.first;
        const auto& data = kv.second;
        ofs.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
        int dim = 1;
        if (!data.empty() && vertices.size() / 3 == data.size()) dim = 1;
        else if (!data.empty() && data.size() % (vertices.size() / 3) == 0) dim = data.size() / (vertices.size() / 3);
        offsets.fields[name] = {current_offset, data.size() * sizeof(float), "float32", dim};
        current_offset += data.size() * sizeof(float);
    }
    ofs.close();
    return true;
}

// Write manifest.json
// Classify geometry kind based on simple heuristics
static string classify_geometry_kind(vtkPolyData* poly, const vector<float>& vertices, const vector<uint32_t>& indices, const map<string, vector<float>>& scalar_data, const string& baseName) {
    if(poly) {
        bool hasLines = poly->GetLines() && poly->GetLines()->GetNumberOfCells() > 0;
        bool hasPolys = poly->GetPolys() && poly->GetPolys()->GetNumberOfCells() > 0;
        if(hasLines && !hasPolys) return "streamline"; // pure line dataset
        // Bounding box to detect slice (planar)
        if(!vertices.empty()) {
            float minv[3] = {vertices[0], vertices[1], vertices[2]};
            float maxv[3] = {vertices[0], vertices[1], vertices[2]};
            for(size_t i=0;i<vertices.size()/3;i++) {
                for(int j=0;j<3;j++) {
                    float v = vertices[i*3+j];
                    if(v<minv[j]) minv[j]=v;
                    if(v>maxv[j]) maxv[j]=v;
                }
            }
            float ex = maxv[0]-minv[0];
            float ey = maxv[1]-minv[1];
            float ez = maxv[2]-minv[2];
            float diag = std::sqrt(ex*ex+ey*ey+ez*ez);
            float eps = diag * 0.01f + 1e-6f;
            if(diag > 0.f && (ex < eps || ey < eps || ez < eps)) {
                return "slice";
            }
        }
        // isosurface heuristic: polygonal, has scalar arrays and baseName / array contains 'iso'
        if(hasPolys) {
            bool hasScalar = !scalar_data.empty();
            if(hasScalar) {
                auto lowerContains = [](const string& s){
                    string t=s; std::transform(t.begin(), t.end(), t.begin(), ::tolower); return t.find("iso")!=string::npos; };
                if(lowerContains(baseName)) return "isosurface";
                for(const auto& kv: scalar_data) { if(lowerContains(kv.first)) return "isosurface"; }
            }
        }
    }
    return "surface"; // default
}

// New manifest creator accepting geometry kind
bool create_manifest(const vector<float>& vertices, const vector<uint32_t>& indices, const map<string, vector<float>>& scalar_data, const UVFOffsets& offsets, const string& bin_path, const string& name, const string& output_dir, string& manifest_path, const string& geom_kind) {
    std::ostringstream sections_ss;
    sections_ss << "[";
    bool first=true;
    for (const auto& kv : offsets.fields) {
        if(!first) sections_ss << ","; first=false;
        sections_ss << "{\"dType\":\""<<kv.second.dType<<"\",";
        sections_ss << "\"dimension\":"<<kv.second.dimension<<",";
        sections_ss << "\"length\":"<<kv.second.length<<",";
        sections_ss << "\"name\":\""<<kv.first<<"\",";
        sections_ss << "\"offset\":"<<kv.second.offset;
        
        // Add rangeMin and rangeMax for scalar data
        if (kv.first != "indices" && kv.first != "position") {
            auto scalar_it = scalar_data.find(kv.first);
            if (scalar_it != scalar_data.end() && !scalar_it->second.empty()) {
                const auto& data = scalar_it->second;
                float minVal = *std::min_element(data.begin(), data.end());
                float maxVal = *std::max_element(data.begin(), data.end());
                sections_ss << ",\"rangeMin\":" << minVal;
                sections_ss << ",\"rangeMax\":" << maxVal;
            }
        }
        
        sections_ss << "}";
    }
    sections_ss << "]";
    
    // Map geom_kind to valid second layer ID
    string second_layer_id;
    if (geom_kind == "slice") {
        second_layer_id = "slices";
    } else if (geom_kind == "isosurface") {
        second_layer_id = "isosurfaces";
    } else if (geom_kind == "streamline") {
        second_layer_id = "streamlines";
    } else {
        second_layer_id = "surfaces"; // default for "surface" and any other cases
    }
    
    std::ostringstream manifest_ss;
    manifest_ss << "[";
    // First layer: root_group (GeometryGroup)
    manifest_ss << "{\"attributions\":{\"members\":[\""<<second_layer_id<<"\"]},\"id\":\"root_group\",\"properties\":{\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],\"type\":0},\"type\":\"GeometryGroup\"},";
    // Second layer: adapt attributions for line (streamline) data: put name under edges instead of faces
    if(geom_kind == "streamline") {
        manifest_ss << "{\"attributions\":{\"edges\":[\""<<name<<"\"],\"faces\":[],\"vertices\":[]},\"id\":\""<<second_layer_id<<"\",\"properties\":{\"geomKind\":\""<<geom_kind<<"\"},\"resources\":{\"buffers\":{\"path\":\""<<bin_path<<"\",\"sections\":"<<sections_ss.str()<<",\"type\":\"buffers\"}},\"type\":\"SolidGeometry\"},";
    } else {
        manifest_ss << "{\"attributions\":{\"edges\":[],\"faces\":[\""<<name<<"\"],\"vertices\":[]},\"id\":\""<<second_layer_id<<"\",\"properties\":{\"geomKind\":\""<<geom_kind<<"\"},\"resources\":{\"buffers\":{\"path\":\""<<bin_path<<"\",\"sections\":"<<sections_ss.str()<<",\"type\":\"buffers\"}},\"type\":\"SolidGeometry\"},";
    }
    // Third layer: Face - endIndex should be the total number of indices, not triangles
    manifest_ss << "{\"attributions\":{\"packedParentId\":\""<<second_layer_id<<"\"},\"id\":\""<<name<<"\",\"properties\":{\"alpha\":1,\"bufferLocations\":{\"indices\":[{\"bufNum\":0,\"endIndex\":"<< indices.size() <<",\"startIndex\":0}]},\"color\":16777215,\"geomKind\":\""<<geom_kind<<"\"},\"type\":\"Face\"}]";
    
    manifest_path = output_dir + "/manifest.json";
    std::ofstream ofs(manifest_path);
    if (!ofs) return false;
    ofs << manifest_ss.str();
    ofs.close();
    return true;
}

// New manifest creator supporting multiple face segments
static bool create_manifest_with_faces(const vector<float>& vertices,
                                       const vector<uint32_t>& indices,
                                       const map<string, vector<float>>& scalar_data,
                                       const UVFOffsets& offsets,
                                       const string& bin_path,
                                       const string& baseName,
                                       const string& output_dir,
                                       string& manifest_path,
                                       const string& geom_kind,
                                       const vector<UVFFaceSegment>& faces) {
    // Build sections JSON (same as original)
    std::ostringstream sections_ss;
    sections_ss << "[";
    bool first=true;
    for (const auto& kv : offsets.fields) {
        if(!first) sections_ss << ","; first=false;
        sections_ss << "{\"dType\":\""<<kv.second.dType<<"\",";
        sections_ss << "\"dimension\":"<<kv.second.dimension<<",";
        sections_ss << "\"length\":"<<kv.second.length<<",";
        sections_ss << "\"name\":\""<<kv.first<<"\",";
        sections_ss << "\"offset\":"<<kv.second.offset;
        if (kv.first != "indices" && kv.first != "position") {
            auto it = scalar_data.find(kv.first);
            if(it!=scalar_data.end() && !it->second.empty()) {
                float minV = *std::min_element(it->second.begin(), it->second.end());
                float maxV = *std::max_element(it->second.begin(), it->second.end());
                sections_ss << ",\"rangeMin\":"<<minV;
                sections_ss << ",\"rangeMax\":"<<maxV;
            }
        }
        sections_ss << "}";
    }
    sections_ss << "]";

    // Determine second layer id same as original
    string second_layer_id;
    if (geom_kind == "slice") second_layer_id = "slices";
    else if (geom_kind == "isosurface") second_layer_id = "isosurfaces";
    else if (geom_kind == "streamline") second_layer_id = "streamlines"; // segmentation unlikely but keep path
    else second_layer_id = "surfaces";

    // Collect face ids for attributions (faces vs edges for streamline)
    std::ostringstream manifest_ss;
    manifest_ss << "[";
    // root group
    manifest_ss << "{\"attributions\":{\"members\":[\""<<second_layer_id<<"\"]},\"id\":\"root_group\",\"properties\":{\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],\"type\":0},\"type\":\"GeometryGroup\"},";

    // Build faces list id array string for SolidGeometry attributions
    std::ostringstream faceIdArray;
    faceIdArray << "[";
    for(size_t i=0;i<faces.size();++i){ if(i) faceIdArray << ","; faceIdArray << "\""<<faces[i].id<<"\""; }
    faceIdArray << "]";

    if (geom_kind == "streamline") {
        // place ids under edges
        manifest_ss << "{\"attributions\":{\"edges\":"<<faceIdArray.str()<<",\"faces\":[],\"vertices\":[]},";
    } else {
        manifest_ss << "{\"attributions\":{\"edges\":[],\"faces\":"<<faceIdArray.str()<<",\"vertices\":[]},";
    }
    manifest_ss << "\"id\":\""<<second_layer_id<<"\",\"properties\":{\"geomKind\":\""<<geom_kind<<"\"},\"resources\":{\"buffers\":{\"path\":\""<<bin_path<<"\",\"sections\":"<<sections_ss.str()<<",\"type\":\"buffers\"}},\"type\":\"SolidGeometry\"},";

    // Each face segment
    for(size_t i=0;i<faces.size();++i){
        const auto& f = faces[i];
        manifest_ss << "{\"attributions\":{\"packedParentId\":\""<<second_layer_id<<"\"},\"id\":\""<<f.id<<"\",\"properties\":{\"alpha\":1,\"bufferLocations\":{\"indices\":[{\"bufNum\":0,\"endIndex\":"<<f.endIndex<<",\"startIndex\":"<<f.startIndex<<"}]},\"color\":16777215,\"geomKind\":\""<<geom_kind<<"\"},\"type\":\"Face\"}";
        if(i+1<faces.size()) manifest_ss << ",";
    }
    manifest_ss << "]";

    manifest_path = output_dir + "/manifest.json";
    std::ofstream ofs(manifest_path);
    if(!ofs) return false;
    ofs << manifest_ss.str();
    ofs.close();
    return true;
}

// Backwards compatibility wrapper (defaults to surface)
bool create_manifest(const vector<float>& vertices, const vector<uint32_t>& indices, const map<string, vector<float>>& scalar_data, const UVFOffsets& offsets, const string& bin_path, const string& name, const string& output_dir, string& manifest_path) {
    return create_manifest(vertices, indices, scalar_data, offsets, bin_path, name, output_dir, manifest_path, "surface");
}

// Extract geometry data helper function (for structured parser)
bool extract_geometry_data(vtkPolyData* polydata, vector<float>& vertices, vector<uint32_t>& indices, map<string, vector<float>>& scalar_data) {
    if (!polydata || !polydata->GetPoints()) return false;
    
    vertices.clear();
    indices.clear();
    scalar_data.clear();
    
    // Extract vertices
    auto pts = polydata->GetPoints();
    vtkIdType nPts = pts->GetNumberOfPoints();
    vertices.resize(nPts * 3);
    for (vtkIdType i = 0; i < nPts; ++i) {
        double p[3];
        pts->GetPoint(i, p);
        vertices[i * 3 + 0] = static_cast<float>(p[0]);
        vertices[i * 3 + 1] = static_cast<float>(p[1]);
        vertices[i * 3 + 2] = static_cast<float>(p[2]);
    }
    
    // Triangulate polys
    auto polys = polydata->GetPolys();
    auto idList = vtkSmartPointer<vtkIdList>::New();
    polys->InitTraversal();
    while (polys->GetNextCell(idList)) {
        if (idList->GetNumberOfIds() < 3) continue;
        for (vtkIdType j = 1; j < idList->GetNumberOfIds() - 1; ++j) {
            indices.push_back(static_cast<uint32_t>(idList->GetId(0)));
            indices.push_back(static_cast<uint32_t>(idList->GetId(j)));
            indices.push_back(static_cast<uint32_t>(idList->GetId(j + 1)));
        }
    }
    // Fallback for pure line/polyline data
    if(indices.empty() && polydata->GetLines() && polydata->GetLines()->GetNumberOfCells()>0){
        auto lines = polydata->GetLines();
        lines->InitTraversal();
        while(lines->GetNextCell(idList)){
            if(idList->GetNumberOfIds()<2) continue;
            for(vtkIdType j=0;j<idList->GetNumberOfIds()-1;++j){
                uint32_t a = static_cast<uint32_t>(idList->GetId(j));
                uint32_t b = static_cast<uint32_t>(idList->GetId(j+1));
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(b);
            }
        }
    }
    
    // Extract scalar fields
    auto pd = polydata->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        auto arr = pd->GetArray(i);
        if (!arr) continue;
        string name = arr->GetName() ? arr->GetName() : ("field" + std::to_string(i));
        int nComp = arr->GetNumberOfComponents();
        vtkIdType nTuples = arr->GetNumberOfTuples();
        vector<float> data(nTuples * nComp);
        for (vtkIdType t = 0; t < nTuples; ++t) {
            for (int c = 0; c < nComp; ++c) {
                data[t * nComp + c] = static_cast<float>(arr->GetComponent(t, c));
            }
        }
        scalar_data[name] = std::move(data);
    }
    
    return true;
}

// 高级 UVF 生成主流程
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir) {
    if (!poly) return false;
    vector<float> vertices;
    vector<uint32_t> indices;
    map<string, vector<float>> scalar_data;
    if(!poly->GetPoints()) return false;
    
    // 提取数据
    auto pts = poly->GetPoints();
    vtkIdType nPts = pts->GetNumberOfPoints();
    vertices.resize(nPts * 3);
    for (vtkIdType i = 0; i < nPts; ++i) {
        double p[3];
        pts->GetPoint(i, p);
        vertices[i * 3 + 0] = static_cast<float>(p[0]);
        vertices[i * 3 + 1] = static_cast<float>(p[1]);
        vertices[i * 3 + 2] = static_cast<float>(p[2]);
    }
    
    // Face segmentation detection (CellData: FaceIndex; FieldData: FaceIdMapping)
    vtkDataArray* faceIndexArr = poly->GetCellData() ? poly->GetCellData()->GetArray("FaceIndex") : nullptr; // could be vtkIntArray etc.
    std::vector<std::string> faceNameMap;
    if(poly->GetFieldData()){
        for(int ai=0; ai<poly->GetFieldData()->GetNumberOfArrays(); ++ai){
            vtkAbstractArray* arr = poly->GetFieldData()->GetAbstractArray(ai);
            if(!arr || !arr->GetName()) continue;
            if(std::string(arr->GetName())=="FaceIdMapping"){
                if(auto sArr = vtkStringArray::SafeDownCast(arr)){
                    for(vtkIdType v=0; v<sArr->GetNumberOfValues(); ++v){ faceNameMap.push_back(sArr->GetValue(v)); }
                }
                break; // stop after mapping array
            }
        }
    }
    std::map<int, std::vector<uint32_t>> faceIndexBuckets; // faceIdx -> local indices
    bool useSegmentation = faceIndexArr != nullptr;
    auto polys = poly->GetPolys();
    auto idList = vtkSmartPointer<vtkIdList>::New();
    polys->InitTraversal();
    vtkIdType cellId = 0;
    while (polys->GetNextCell(idList)) {
        if (idList->GetNumberOfIds() < 3) { cellId++; continue; }
        int fIdx = 0;
        if(useSegmentation && cellId < faceIndexArr->GetNumberOfTuples()) {
            fIdx = static_cast<int>(faceIndexArr->GetComponent(cellId,0));
        }
        for (vtkIdType j = 1; j < idList->GetNumberOfIds() - 1; ++j) {
            uint32_t a = static_cast<uint32_t>(idList->GetId(0));
            uint32_t b = static_cast<uint32_t>(idList->GetId(j));
            uint32_t c = static_cast<uint32_t>(idList->GetId(j + 1));
            if(useSegmentation) {
                auto& bucket = faceIndexBuckets[fIdx];
                bucket.push_back(a); bucket.push_back(b); bucket.push_back(c);
            } else {
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
            }
        }
        cellId++;
    }
    // Fallback: lines only
    if(indices.empty() && poly->GetLines() && poly->GetLines()->GetNumberOfCells()>0){
        auto lines = poly->GetLines();
        lines->InitTraversal();
        while(lines->GetNextCell(idList)){
            if(idList->GetNumberOfIds()<2) continue;
            for(vtkIdType j=0;j<idList->GetNumberOfIds()-1;++j){
                uint32_t a = static_cast<uint32_t>(idList->GetId(j));
                uint32_t b = static_cast<uint32_t>(idList->GetId(j+1));
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(b);
            }
        }
    }

    // If segmentation present, flatten buckets into indices and build segment metadata
    std::vector<UVFFaceSegment> segments;
    if(useSegmentation && !faceIndexBuckets.empty()){
        indices.clear();
        indices.reserve([&](){ size_t total=0; for(auto& kv: faceIndexBuckets) total += kv.second.size(); return total; }());
        std::vector<int> sortedKeys; sortedKeys.reserve(faceIndexBuckets.size());
        for(auto& kv: faceIndexBuckets) sortedKeys.push_back(kv.first);
        std::sort(sortedKeys.begin(), sortedKeys.end());
        size_t cursor = 0;
        for(int key : sortedKeys){
            auto& vec = faceIndexBuckets[key];
            if(vec.empty()) continue;
            size_t start = cursor;
            indices.insert(indices.end(), vec.begin(), vec.end());
            cursor += vec.size();
            size_t end = cursor;
            UVFFaceSegment seg;
            if(key >=0 && key < static_cast<int>(faceNameMap.size()) && !faceNameMap[key].empty()) seg.id = faceNameMap[key];
            else { std::ostringstream oss; oss << "uvf_Face" << key; seg.id = oss.str(); }
            seg.startIndex = start; seg.endIndex = end; segments.push_back(std::move(seg));
        }
        if(segments.empty()) useSegmentation = false; // fallback
    }
    
    auto pd = poly->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        auto arr = pd->GetArray(i);
        if (!arr) continue;
        string name = arr->GetName() ? arr->GetName() : ("field" + std::to_string(i));
        int nComp = arr->GetNumberOfComponents();
        vtkIdType nTuples = arr->GetNumberOfTuples();
        vector<float> data(nTuples * nComp);
        
        for (vtkIdType t = 0; t < nTuples; ++t) {
            for (int c = 0; c < nComp; ++c) {
                float val = static_cast<float>(arr->GetComponent(t, c));
                data[t * nComp + c] = val;
            }
        }
        
        scalar_data[name] = std::move(data);
    }
    
    // 目录结构
    string out_dir = string(uvf_dir);
    string resources_dir = out_dir + "/";
    make_dirs(out_dir);
    make_dirs(resources_dir);
    // generate random bin file name
    std::string rand8 = make_random_token(8);
    string bin_filename = rand8 + ".bin";
    string bin_path = resources_dir + "/" + bin_filename;
    UVFOffsets offsets;
    if (!write_binary_data(vertices, indices, scalar_data, bin_path, offsets)) return false;
    string manifest_path;
    // Determine geometry kind from original polydata & data
    string geomKind = classify_geometry_kind(poly, vertices, indices, scalar_data, "uvf");
    if(useSegmentation && !segments.empty()) {
        if(!create_manifest_with_faces(vertices, indices, scalar_data, offsets, bin_filename, "uvf", out_dir, manifest_path, geomKind, segments)) return false;
    } else {
        if (!create_manifest(vertices, indices, scalar_data, offsets, bin_filename, "uvf", out_dir, manifest_path, geomKind)) return false;
    }
    return true;
}
