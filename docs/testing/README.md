# Testing Documentation

## Test Structure

The project uses a comprehensive testing approach with multiple test suites:

### Test Organization

```
tests/
├── test_data_array_info.cpp    # DataArray metadata extraction tests
├── test_geom_kind.cpp          # Geometry type classification tests
├── test_file_inputs.cpp        # File input format tests
└── data/                       # Test data files
```

### Running Tests

#### Individual Test Suites

```bash
# Build and run geometry classification tests
cd build
make uvf_tests
./uvf_tests

# Build and run file input tests  
make uvf_file_tests
./uvf_file_tests

# Build and run DataArray info tests
make test_data_array_info_example
./test_data_array_info_example
```

#### All Tests
```bash
cd build
make test
```

### Test Categories

#### 1. Geometry Classification Tests (`test_geom_kind.cpp`)
- Tests automatic geometry type detection (slice, surface, streamline)
- Validates manifest.json geomKind field generation
- Creates synthetic test geometries

#### 2. File Input Tests (`test_file_inputs.cpp`)
- Tests various VTP/VTK file formats
- Validates file parsing and geometry type detection
- Uses real test data files

#### 3. DataArray Information Tests (`test_data_array_info.cpp`)
- Tests metadata extraction from VTK data arrays
- Validates component count, tuple count, and value ranges
- Tests both basic and enhanced UVF generation modes

### Test Data

Test data files should be placed in `tests/data/`:
- `slice_sample.vtp` - Sample slice geometry
- `line_sample.vtp` - Sample streamline geometry  
- `surface_sample.vtk` - Sample surface geometry

### Writing New Tests

#### Test Function Template
```cpp
bool test_my_feature() {
    std::cout << "Testing my feature..." << std::endl;
    
    // Setup test data
    auto polydata = create_test_geometry();
    if (!polydata) {
        std::cerr << "Failed to create test geometry" << std::endl;
        return false;
    }
    
    // Run test
    std::string outputDir = "test_output_my_feature";
    system(("rm -rf " + outputDir).c_str());
    
    bool result = my_function(polydata, outputDir.c_str());
    
    // Validate results
    if (!result) {
        std::cerr << "Test function failed" << std::endl;
        return false;
    }
    
    // Additional validations...
    
    std::cout << "Test passed!" << std::endl;
    return true;
}
```

#### Main Function Template
```cpp
int main() {
    std::cout << "=== My Test Suite ===" << std::endl;
    
    bool test1 = test_feature_1();
    bool test2 = test_feature_2();
    
    if (test1 && test2) {
        std::cout << "\n✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n❌ Some tests failed!" << std::endl;
        return 1;
    }
}
```

### Test Guidelines

1. **Isolation**: Each test should clean up its output directory
2. **Validation**: Always validate both success/failure and output correctness
3. **Error Messages**: Provide clear error messages for debugging
4. **Test Data**: Use both synthetic and real test data when appropriate
5. **Coverage**: Test both happy path and error conditions
