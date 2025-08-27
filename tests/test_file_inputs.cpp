#include "vtp_to_uvf.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

static bool file_exists(const std::string& p){ struct stat st; return ::stat(p.c_str(), &st)==0; }
static bool load_kind(const std::string& dir, std::string& kind){ std::ifstream ifs(dir+"/manifest.json"); if(!ifs) return false; std::string c((std::istreambuf_iterator<char>(ifs)),{}); auto pos=c.find("\"geomKind\":\""); if(pos==std::string::npos) return false; pos+=12; auto end=c.find('"',pos); if(end==std::string::npos) return false; kind=c.substr(pos,end-pos); return true; }

int main(){
    std::vector<std::pair<std::string,std::string>> samples = {
        {"slice_sample.vtp","slice"},
        {"line_sample.vtp","streamline"},
        {"surface_sample.vtk","surface"}
    };
    bool all=true; int idx=0;
    for(auto & s : samples){
        std::string path = std::string(TEST_DATA_DIR)+"/"+s.first;
        if(!file_exists(path)) { std::cerr<<"Missing test data: "<<path<<" (skip)\n"; continue; }
        auto poly = parse_vtp_file(path.c_str());
        if(!poly){ std::cerr<<"Failed parse "<<path<<"\n"; all=false; continue; }
        std::string outDir = "file_case_"+std::to_string(idx++);
        system((std::string("rm -rf ")+outDir).c_str());
        if(!generate_uvf(poly, outDir.c_str())) { std::cerr<<"generate_uvf fail "<<path<<"\n"; all=false; continue; }
        std::string kind; if(!load_kind(outDir, kind)) { std::cerr<<"manifest read fail "<<path<<"\n"; all=false; continue; }
        if(kind!=s.second) { std::cerr<<"Kind mismatch for "<<path<<" got="<<kind<<" expect="<<s.second<<"\n"; all=false; }
    }
    if(!all) return 1; 
    std::cout<<"File input tests passed"<<std::endl; 
    return 0;
}
