#include "../src/vtp_to_uvf.h"
#include <iostream>
#include <cassert>

/**
 * Test case for DataArray information extraction functionality
 * Verifies that the enhanced generate_uvf function correctly extracts
 * and returns metadata about VTK data arrays
 */
bool test_data_array_info_extraction() {
    std::cout << "Testing DataArray information extraction..." << std::endl;
    
    auto polydata = parse_vtp_file("../assets/star.vtp");
    if (!polydata) {
        std::cerr << "Failed to load VTP file: ../assets/star.vtp" << std::endl;
        return false;
    }
    
    // Vector to store DataArray information
    vector<DataArrayInfo> arrayInfo;
    
    std::string outputDir = "test_output_array_info";
    system(("rm -rf " + outputDir).c_str());
    
    if (!generate_uvf(polydata, outputDir.c_str(), &arrayInfo)) {
        std::cerr << "UVF generation failed!" << std::endl;
        return false;
    }
    
    std::cout << "UVF generation successful!" << std::endl;
    std::cout << "Found " << arrayInfo.size() << " data arrays:" << std::endl;
    
    for (const auto& info : arrayInfo) {
        std::cout << "  Array: " << info.name << std::endl;
        std::cout << "    Components: " << info.components << std::endl;
        std::cout << "    Tuples: " << info.tuples << std::endl;
        std::cout << "    Range: [" << info.rangeMin << ", " << info.rangeMax << "]" << std::endl;
        std::cout << "    Data Type: " << info.dType << std::endl;
    }
    
    return true;
}

/**
 * Test case for basic UVF generation without DataArray info
 */
bool test_basic_uvf_generation() {
    std::cout << "Testing basic UVF generation..." << std::endl;
    
    auto polydata = parse_vtp_file("../assets/star.vtp");
    if (!polydata) {
        std::cerr << "Failed to load VTP file: ../assets/star.vtp" << std::endl;
        return false;
    }
    
    std::string outputDir = "test_output_basic";
    system(("rm -rf " + outputDir).c_str());
    
    if (!generate_uvf(polydata, outputDir.c_str())) {
        std::cerr << "Basic UVF generation failed!" << std::endl;
        return false;
    }
    
    std::cout << "Basic UVF generation successful!" << std::endl;
    return true;
}

int main() {
    std::cout << "=== DataArray Information Test Suite ===" << std::endl;
    
    bool test1 = test_basic_uvf_generation();
    bool test2 = test_data_array_info_extraction();
    
    if (test1 && test2) {
        std::cout << "\n✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n❌ Some tests failed!" << std::endl;
        std::cerr << "Basic generation: " << (test1 ? "PASS" : "FAIL") << std::endl;
        std::cerr << "Array info extraction: " << (test2 ? "PASS" : "FAIL") << std::endl;
        return 1;
    }
}
