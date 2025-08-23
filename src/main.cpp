#include "vtp_to_uvf.h"
#include <vtkXMLPolyDataReader.h>
#include <vtkSmartPointer.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " input.vtp output_dir" << std::endl;
        return 1;
    }
    const char* vtp_path = argv[1];
    const char* uvf_dir = argv[2];
    auto poly = parse_vtp_file(vtp_path);
    if (!poly) {
        std::cerr << "Failed to read VTP: " << vtp_path << std::endl;
        return 2;
    }
    if (!generate_uvf(poly, uvf_dir)) {
        std::cerr << "Failed to generate UVF in: " << uvf_dir << std::endl;
        return 3;
    }
    std::cout << "Success! Output in: " << uvf_dir << std::endl;
    return 0;
}
