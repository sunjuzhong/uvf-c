#include "vtp_to_uvf.h"
#include "vtk_structured_parser.h"
#include "multi_file_parser.h"
#include <vtkXMLPolyDataReader.h>
#include <vtkSmartPointer.h>
#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " input.[vtp|vtk|stl] output_dir [--structured]" << std::endl;
        std::cout << "   or: " << argv[0] << " input_dir/ output_dir --directory" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported input formats:" << std::endl;
        std::cout << "  .vtp  - VTK XML PolyData" << std::endl;
        std::cout << "  .vtk  - VTK Legacy format" << std::endl;
        std::cout << "  .stl  - STL (ASCII or Binary)" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --structured  Use structured parsing based on field names" << std::endl;
        std::cout << "  --directory   Process all VTK files in input directory with structured parsing" << std::endl;
        return 1;
    }

    const char* input_path = argv[1];
    const char* uvf_dir = argv[2];
    bool use_structured = false;
    bool use_directory = false;

    // Check for flags
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--structured") == 0) {
            use_structured = true;
        } else if (strcmp(argv[i], "--directory") == 0) {
            use_directory = true;
        }
    }

    bool success = false;

    if (use_directory) {
        std::cout << "Processing directory: " << input_path << std::endl;
        success = process_directory_structure(input_path, uvf_dir);
    } else {
        auto poly = parse_vtp_file(input_path);
        if (!poly) {
            std::cerr << "Failed to read input file: " << input_path << std::endl;
            return 2;
        }

        if (use_structured) {
            std::cout << "Using structured parsing..." << std::endl;
            success = generate_structured_uvf(poly, uvf_dir);
        } else {
            std::cout << "Using basic parsing..." << std::endl;
            success = generate_uvf(poly, uvf_dir);
        }
    }

    if (!success) {
        std::cerr << "Failed to generate UVF in: " << uvf_dir << std::endl;
        return 3;
    }

    std::cout << "Success! Output in: " << uvf_dir << std::endl;
    return 0;
}
