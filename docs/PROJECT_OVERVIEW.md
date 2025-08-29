# UVF-C Project Documentation

## Project Overview

UVF-C is a VTK to UVF (Universal Volume Format) converter written in C++ with support for both native and WebAssembly builds. The project provides tools for converting VTP/VTK files to UVF format with enhanced metadata extraction capabilities.

## Features

- **Multi-format Support**: Parse VTP (XML), VTK (legacy), and structured grids
- **DataArray Metadata**: Extract comprehensive information about VTK data arrays
- **Geometry Classification**: Automatic detection of geometry types (surface, slice, streamline)
- **Web Support**: WASM builds for browser-based processing
- **C/C++ APIs**: Both native C++ and C API interfaces
- **Comprehensive Testing**: Full test suite with multiple test categories

## Quick Start

### Prerequisites
- VTK (CommonCore, CommonDataModel, IOCore, IOXML modules)
- CMake >= 3.13
- C++17 compatible compiler

### Building
```bash
git clone <repository>
cd uvf-c
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Basic Usage
```cpp
#include "src/vtp_to_uvf.h"

auto polydata = parse_vtp_file("input.vtp");
if (generate_uvf(polydata, "output_directory")) {
    std::cout << "Conversion successful!" << std::endl;
}
```

## Documentation Structure

### [API Reference](api/)
Detailed documentation of all public functions and data structures.

### [Examples](examples/)
Practical usage examples and tutorials for common use cases.

### [Testing](testing/)
Testing guidelines, test structure documentation, and how to add new tests.

### [Development](development/)
Development setup, architecture overview, and contribution guidelines.

## Key Components

### Core Libraries
- `vtp_to_uvf`: Main VTP/VTK to UVF conversion engine
- `vtk_structured_parser`: Structured grid processing
- `multi_file_parser`: Batch file processing
- `uvf_c_api`: C API wrapper for cross-language compatibility

### Test Suite
- **Geometry Tests**: Validate geometry type classification
- **File Input Tests**: Test various VTK file formats
- **DataArray Tests**: Verify metadata extraction functionality

### Build Targets
- **Native**: `uvf_cli` (command-line tool), `libuvf.a` (static library)
- **WASM**: `uvf.js` + `uvf.wasm` for web deployment

## Advanced Features

### DataArray Information Extraction
The enhanced `generate_uvf` function can extract detailed metadata:
```cpp
vector<DataArrayInfo> arrayInfo;
generate_uvf(polydata, "output", &arrayInfo);

for (const auto& info : arrayInfo) {
    std::cout << "Array: " << info.name 
              << ", Range: [" << info.rangeMin << ", " << info.rangeMax << "]"
              << std::endl;
}
```

### Multi-file Processing
Process entire directories of VTK files:
```cpp
process_directory_structure("input_dir", "output_uvf_dir");
```

### WASM Integration
Use in web applications:
```javascript
import UVFModule from './uvf.js';

UVFModule().then(Module => {
    const result = Module.convertVTPToUVF(vtpData);
    console.log('Conversion result:', result);
});
```

## Contributing

1. **Setup Development Environment**: See [development guide](development/)
2. **Write Tests**: Add appropriate tests for new features
3. **Update Documentation**: Keep documentation current with changes
4. **Follow Code Style**: Use consistent naming and formatting
5. **Test Thoroughly**: Verify both native and WASM builds

## Support

- **Issues**: Report bugs and feature requests via GitHub issues
- **Documentation**: Comprehensive guides in the `docs/` directory
- **Examples**: Working code samples in `docs/examples/`

## License

[Add license information here]
