# Development Guide

## Project Architecture

### Core Components

```
src/
├── vtp_to_uvf.h/cpp         # Core VTP/VTK to UVF conversion
├── vtk_structured_parser.h/cpp # Structured grid parsing
├── multi_file_parser.h/cpp  # Multi-file processing
├── uvf_c_api.h/cpp         # C API wrapper
├── uvf_js_bindings.js      # JavaScript bindings for WASM
├── id_utils.h/cpp          # ID utilities
└── main.cpp                # CLI application
```

### Build System

The project uses CMake with support for:
- Native C++ builds
- Emscripten/WASM builds
- VTK integration
- Test compilation

### Dependencies

#### Required
- VTK (CommonCore, CommonDataModel, IOCore, IOXML modules)
- CMake >= 3.13
- C++17 compatible compiler

#### Optional
- Emscripten (for WASM builds)
- nlohmann/json (bundled in third_party/)

### Development Workflow

#### Setting Up Development Environment

1. **Clone and setup:**
   ```bash
   git clone <repository>
   cd uvf-c
   git submodule update --init --recursive
   ```

2. **Install VTK:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libvtk9-dev
   
   # macOS with Homebrew
   brew install vtk
   
   # Or build from source (see VTK documentation)
   ```

3. **Build native version:**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

#### Code Style Guidelines

1. **Naming Conventions:**
   - Functions: `snake_case`
   - Variables: `snake_case` or `camelCase`
   - Classes/Structs: `PascalCase`
   - Constants: `UPPER_CASE`

2. **File Organization:**
   - Header files: `.h` extension
   - Implementation: `.cpp` extension
   - One class per file when possible

3. **Documentation:**
   - Use English comments only
   - Document public APIs
   - Minimal comments for obvious code

#### Adding New Features

1. **API Design:**
   - Consider backward compatibility
   - Use const-correctness
   - Provide both C++ and C APIs when appropriate

2. **Testing:**
   - Add tests for new functionality
   - Test both success and error paths
   - Use appropriate test data

3. **Documentation:**
   - Update API documentation
   - Add usage examples
   - Update README if needed

### WASM Build Process

#### Setup Emscripten
```bash
# Install emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

#### Build VTK for WASM
```bash
./scripts/bootstrap_wasm.sh
```

#### Build UVF-C for WASM
```bash
./scripts/build_wasm.sh
```

### Debugging Tips

1. **Native Debugging:**
   ```bash
   cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   gdb ./uvf_cli
   ```

2. **Memory Issues:**
   ```bash
   valgrind --tool=memcheck ./uvf_cli input.vtp output/
   ```

3. **WASM Debugging:**
   - Use browser developer tools
   - Enable WASM debugging in browser settings
   - Use `console.log` in JavaScript bindings

### Performance Considerations

1. **Memory Management:**
   - Use VTK smart pointers
   - Avoid memory leaks in C API
   - Clean up temporary files

2. **Large Files:**
   - Stream processing when possible
   - Use memory mapping for large datasets
   - Consider chunked processing

3. **WASM Limitations:**
   - Limited memory (1-2GB typically)
   - No direct file system access
   - Use virtual file system appropriately
