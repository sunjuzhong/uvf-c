cmake_minimum_required(VERSION 3.13)
project(uvf_converter)

# Emscripten toolchain
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Find VTK
find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    IOXML
    FiltersCore
)

include(${VTK_USE_FILE})


add_library(uvf STATIC
    src/vtp_to_uvf.cpp
    src/uvf_c_api.cpp
)
add_executable(uvf_cli src/main.cpp src/vtp_to_uvf.cpp)

target_include_directories(uvf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(uvf PRIVATE ${VTK_LIBRARIES})
target_link_libraries(uvf_cli PRIVATE ${VTK_LIBRARIES})

# Emscripten: build WASM and JS glue
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set_target_properties(uvf PROPERTIES
        OUTPUT_NAME "uvf"
        SUFFIX ".wasm"
        LINK_FLAGS "--bind -s MODULARIZE=1 -s EXPORT_NAME=\"UVFModule\" -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=['_parse_vtp','_generate_uvf'] -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    )
endif()
