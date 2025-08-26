
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
#include <sys/stat.h>
#include <sys/types.h>

using std::vector;
using std::string;
using std::map;

struct UVFOffsets {
    struct Info {
        size_t offset;
        size_t length;
        string dType;
        int dimension;
    };
    map<string, Info> fields;
};

// Detect extension helper
static std::string file_ext_lower(const char* path){
    std::string s(path?path:"");
    auto pos = s.find_last_of('.');
    if(pos==std::string::npos) return ""; 
    std::string ext = s.substr(pos+1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
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
bool create_manifest(const vector<float>& vertices, const vector<uint32_t>& indices, const map<string, vector<float>>& scalar_data, const UVFOffsets& offsets, const string& bin_path, const string& name, const string& output_dir, string& manifest_path) {
    // manual json assembly (avoid external dependency in minimal wasm)
    float min_coords[3] = {vertices[0], vertices[1], vertices[2]};
    float max_coords[3] = {vertices[0], vertices[1], vertices[2]};
    for (size_t i = 0; i < vertices.size() / 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            min_coords[j] = std::min(min_coords[j], vertices[i * 3 + j]);
            max_coords[j] = std::max(max_coords[j], vertices[i * 3 + j]);
        }
    }
    std::ostringstream sections_ss;
    sections_ss << "[";
    bool first=true;
    for (const auto& kv : offsets.fields) {
        if(!first) sections_ss << ","; first=false;
        sections_ss << "{\"dType\":\""<<kv.second.dType<<"\",";
        sections_ss << "\"dimension\":"<<kv.second.dimension<<",";
        sections_ss << "\"length\":"<<kv.second.length<<",";
        sections_ss << "\"name\":\""<<kv.first<<"\",";
        sections_ss << "\"offset\":"<<kv.second.offset<<"}";
    }
    sections_ss << "]";
    string slice_id = name + "-slice";
    std::ostringstream manifest_ss;
    manifest_ss << "[";
    manifest_ss << "{\"attributions\":{\"members\":[\""<<slice_id<<"\"]},\"id\":\"root_group\",\"properties\":{\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],\"type\":0},\"type\":\"GeometryGroup\"},";
    manifest_ss << "{\"attributions\":{\"edges\":[],\"faces\":[\""<<name<<"\"],\"vertices\":[]},\"id\":\""<<slice_id<<"\",\"properties\":{},\"resources\":{\"buffers\":{\"path\":\""<<bin_path<<"\",\"sections\":"<<sections_ss.str()<<",\"type\":\"buffers\"}},\"type\":\"SolidGeometry\"},";
    manifest_ss << "{\"attributions\":{\"packedParentId\":\""<<slice_id<<"\"},\"id\":\""<<name<<"\",\"properties\":{\"alpha\":1,\"bufferLocations\":{\"indices\":[{\"bufNum\":0,\"endIndex\":"<< (indices.size()/3) <<",\"startIndex\":0}]},\"color\":16777215},\"type\":\"Face\"}]";
    manifest_path = output_dir + "/manifest.json";
    std::ofstream ofs(manifest_path);
    if (!ofs) return false;
    ofs << manifest_ss.str();
    ofs.close();
    return true;
}

// 递归创建目录
inline void make_dirs(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0777);
#endif
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
    auto polys = poly->GetPolys();
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
                data[t * nComp + c] = static_cast<float>(arr->GetComponent(t, c));
            }
        }
        scalar_data[name] = std::move(data);
    }
    // 目录结构
    string out_dir = string(uvf_dir);
    string resources_dir = out_dir + "/resources/uvf";
    make_dirs(out_dir);
    make_dirs(out_dir + "/resources");
    make_dirs(resources_dir);
    string bin_path = resources_dir + "/uvf.bin";
    UVFOffsets offsets;
    if (!write_binary_data(vertices, indices, scalar_data, bin_path, offsets)) return false;
    string manifest_path;
    if (!create_manifest(vertices, indices, scalar_data, offsets, "resources/uvf/uvf.bin", "uvf", out_dir, manifest_path)) return false;
    return true;
}
