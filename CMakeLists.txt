cmake_minimum_required(VERSION 3.13)
project(uvf_converter)

option(UVF_BUILD_CLI "Build native CLI" ON)


# Emscripten toolchain
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Find VTK (native & wasm both need a build of VTK; for wasm you must pre-build VTK with emscripten toolchain)
find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    IOXML
    IOLegacy
    FiltersGeometry
)

if(NOT VTK_FOUND)
    message(FATAL_ERROR "VTK not found with required minimal components")
endif()

if(TARGET VTK::FiltersCore)
    list(APPEND VTK_LIBRARIES VTK::FiltersCore)
else()
    message(STATUS "VTK::FiltersCore not present; proceeding without it (ensure triangulation logic does not rely on FiltersCore).")
endif()

include(${VTK_USE_FILE})

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    # Wasm build: produce a JS+WASM module exporting C API
    add_executable(uvf_wasm
        src/vtp_to_uvf.cpp
        src/vtk_structured_parser.cpp
        src/multi_file_parser.cpp
        src/id_utils.cpp
        src/uvf_c_api.cpp
    )
    # Provide third_party/nlohmann if exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
        target_include_directories(uvf_wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
    endif()
    target_include_directories(uvf_wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(uvf_wasm PRIVATE ${VTK_LIBRARIES})
    set_target_properties(uvf_wasm PROPERTIES
        OUTPUT_NAME "uvf"
        LINK_FLAGS "--bind -s MODULARIZE=1 -s EXPORT_NAME=\"UVFModule\" -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s ENVIRONMENT=web,worker -s EXPORTED_FUNCTIONS=['_parse_vtp','_generate_uvf','_uvf_get_last_error','_uvf_get_last_point_count','_uvf_get_last_triangle_count'] -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','lengthBytesUTF8','stringToUTF8','UTF8ToString']"
    )
else()
    # Native build: static lib + CLI tool
    add_library(uvf STATIC
        src/vtp_to_uvf.cpp
        src/vtk_structured_parser.cpp
        src/multi_file_parser.cpp
        src/id_utils.cpp
        src/uvf_c_api.cpp
    )
    if(UVF_BUILD_CLI)
        add_executable(uvf_cli 
            src/main.cpp 
            src/vtp_to_uvf.cpp
            src/vtk_structured_parser.cpp
            src/multi_file_parser.cpp
            src/id_utils.cpp
        )
        # Provide third_party/nlohmann if exists
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
            target_include_directories(uvf_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
        endif()
        target_link_libraries(uvf_cli PRIVATE ${VTK_LIBRARIES})
    endif()
    target_include_directories(uvf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    # Provide third_party/nlohmann if exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
        target_include_directories(uvf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include)
    endif()
    target_link_libraries(uvf PRIVATE ${VTK_LIBRARIES})
endif()
