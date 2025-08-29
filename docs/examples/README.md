# Examples

## Basic Usage

### Simple VTP to UVF Conversion
```cpp
#include "src/vtp_to_uvf.h"
#include <iostream>

int main() {
    // Parse VTP file
    auto polydata = parse_vtp_file("input.vtp");
    if (!polydata) {
        std::cerr << "Failed to parse VTP file" << std::endl;
        return -1;
    }
    
    // Generate UVF
    if (generate_uvf(polydata, "output_directory")) {
        std::cout << "UVF generation successful!" << std::endl;
    } else {
        std::cout << "UVF generation failed!" << std::endl;
        return -1;
    }
    
    return 0;
}
```

### Enhanced UVF Generation with DataArray Information
```cpp
#include "src/vtp_to_uvf.h"
#include <iostream>

int main() {
    auto polydata = parse_vtp_file("input.vtp");
    if (!polydata) {
        std::cerr << "Failed to parse VTP file" << std::endl;
        return -1;
    }
    
    // Vector to store DataArray information
    vector<DataArrayInfo> arrayInfo;
    
    // Generate UVF with metadata extraction
    if (generate_uvf(polydata, "output_directory", &arrayInfo)) {
        std::cout << "UVF generation successful!" << std::endl;
        std::cout << "Found " << arrayInfo.size() << " data arrays:" << std::endl;
        
        for (const auto& info : arrayInfo) {
            std::cout << "Array: " << info.name << std::endl;
            std::cout << "  Components: " << info.components << std::endl;
            std::cout << "  Tuples: " << info.tuples << std::endl;
            std::cout << "  Range: [" << info.rangeMin << ", " << info.rangeMax << "]" << std::endl;
            std::cout << "  Type: " << info.dType << std::endl;
        }
    } else {
        std::cout << "UVF generation failed!" << std::endl;
        return -1;
    }
    
    return 0;
}
```

## Building Examples

### CMake Configuration
Add to your CMakeLists.txt:
```cmake
find_package(VTK REQUIRED COMPONENTS CommonCore CommonDataModel IOCore IOXML)

add_executable(my_example example.cpp)
target_link_libraries(my_example ${VTK_LIBRARIES} uvf)
```

### Compilation
```bash
mkdir build && cd build
cmake ..
make
```
