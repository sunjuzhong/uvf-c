# UVF-C: VTK to UVF Converter

A high-performance C++ library for converting VTK/VTP files to Universal Volume Format (UVF) with support for both native and WebAssembly builds.

## Features

- **Multi-format Input**: Support for VTP (XML), VTK (legacy), and structured grids
- **DataArray Metadata**: Extract comprehensive information about data arrays (ranges, types, components)
- **Geometry Classification**: Automatic detection of geometry types (surface, slice, streamline)
- **Cross-platform**: Native C++ and WebAssembly builds
- **Comprehensive APIs**: Both C++ and C interfaces
- **Robust Testing**: Full test suite with multiple test categories

## Quick Start

### Prerequisites
- VTK (CommonCore, CommonDataModel, IOCore, IOXML modules)
- CMake >= 3.13
- C++17 compatible compiler
- Emscripten (for WASM builds)

### Building (Native C++)

```bash
git clone <repository>
cd uvf-c
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Building (WebAssembly)

**快速开始** (推荐):
```bash
# 激活 emsdk 环境
source ~/emsdk/emsdk_env.sh

# 一键构建并启动开发服务器
./scripts/dev-wasm.sh --serve
```

首次构建需编译 VTK 依赖（约 20-40 分钟），后续构建仅需 1-2 分钟。

📖 **详细指南**: [本地 WASM 开发文档](docs/wasm/LOCAL_DEVELOPMENT.md)

<details>
<summary>手动构建步骤</summary>

```bash
# Setup Emscripten
source /path/to/emsdk/emsdk_env.sh

# Build VTK for WASM (first time only)
./scripts/bootstrap_wasm.sh

# Build UVF-C for WASM
./scripts/build_wasm.sh
```
</details>

## Usage

### Command Line Interface
```bash
# Convert single file
./uvf_cli input.vtp output_directory

# Process directory of files
./uvf_cli input_directory/ output_directory/ directory

# Structured grid processing
./uvf_cli input.vtk output_directory structured
```

### C++ API
```cpp
#include "src/vtp_to_uvf.h"

// Basic conversion
auto polydata = parse_vtp_file("input.vtp");
generate_uvf(polydata, "output_directory");

// With DataArray metadata extraction
vector<DataArrayInfo> arrayInfo;
generate_uvf(polydata, "output_directory", &arrayInfo);

for (const auto& info : arrayInfo) {
    std::cout << "Array: " << info.name
              << ", Range: [" << info.rangeMin << ", " << info.rangeMax << "]"
              << std::endl;
}
```

### JavaScript API (WebAssembly)
```javascript
import UVFModule from './uvf.js';

UVFModule().then(Module => {
    // Load VTP file to virtual filesystem
    Module.FS_createDataFile('/', 'input.vtp', vtpData, true, true);

    // Convert to UVF
    const result = Module.ccall('generate_uvf', 'number',
        ['string', 'string'], ['/input.vtp', '/output']);

    if (result) {
        // Read generated UVF files
        const manifest = Module.FS_readFile('/output/manifest.json');
        const binaryData = Module.FS_readFile('/output/resources/data.bin');
    }
});
```

## Testing

### Run All Tests
```bash
cd build
make test
```

### Individual Test Suites
```bash
# Geometry classification tests
./uvf_tests

# File input format tests
./uvf_file_tests

# DataArray metadata tests
./uvf_data_array_tests
```

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **[Project Overview](docs/PROJECT_OVERVIEW.md)**: Complete project documentation
- **[API Reference](docs/api/)**: Detailed function and data structure documentation
- **[Examples](docs/examples/)**: Usage examples and tutorials
- **[Testing Guide](docs/testing/)**: Testing guidelines and test structure
- **[Development Guide](docs/development/)**: Architecture and development setup

## Project Structure

```
uvf-c/
├── src/                     # Source code
│   ├── vtp_to_uvf.{h,cpp}  # Core conversion engine
│   ├── uvf_c_api.{h,cpp}   # C API wrapper
│   └── main.cpp            # CLI application
├── tests/                   # Test suite
│   ├── test_geom_kind.cpp  # Geometry classification tests
│   ├── test_file_inputs.cpp # File format tests
│   └── test_data_array_info.cpp # DataArray metadata tests
├── docs/                    # Documentation
├── scripts/                 # Build scripts
├── assets/                  # Sample data files
└── demo/                    # Web demo
```

## Build Targets

### Native Builds
- `libuvf.a`: Static library
- `uvf_cli`: Command-line tool
- `uvf_tests`, `uvf_file_tests`, `uvf_data_array_tests`: Test executables

### WebAssembly Builds
- `uvf.js`: JavaScript glue code
- `uvf.wasm`: WebAssembly binary

## Contributing

1. **Setup**: Follow the [development guide](docs/development/)
2. **Tests**: Add tests for new features
3. **Documentation**: Update relevant documentation
4. **Code Style**: Use consistent formatting and English comments
5. **Review**: Test both native and WASM builds

## License

[Add license information]
```sh
python3 -m http.server 8080
```
4. 浏览器访问 http://localhost:8080 ，选择 .vtp 转换

若需要 Worker 模式，可把 import 改到 worker 内并通过 postMessage 传递 ArrayBuffer。

## 说明
- 你可以在 `src/vtp_to_uvf.cpp` 实现更复杂的 VTP 解析和 UVF 生成逻辑。
- C 接口在 `src/uvf_c_api.cpp`，方便 Emscripten 导出。
- CMakeLists.txt 已配置 VTK、Emscripten、WASM 导出参数。
