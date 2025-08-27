# WASM 编译流程总结

## 步骤总览

```mermaid
flowchart TD
A[准备 emsdk 环境] --> B[编译 VTK wasm 依赖]
B --> C[配置主项目 (emcmake cmake)]
C --> D[编译主项目 (cmake --build)]
D --> E[收集 wasm 产物]
```

## 详细步骤


1. **准备 emsdk 环境**
   - 目录位置：`emsdk` 建议放在项目根目录（即 `uvf-c/emsdk`）
   - 在项目根目录下执行：
     ```sh
     git clone https://github.com/emscripten-core/emsdk.git
     cd emsdk
     ./emsdk install 3.1.64
     ./emsdk activate 3.1.64
     cd ..
     source emsdk/emsdk_env.sh
     ```


2. **编译 VTK wasm 依赖**
   - 目录位置：`third_party` 建议放在项目根目录（即 `uvf-c/third_party`）
   - 在项目根目录下执行：
     ```sh
     mkdir -p third_party
     git clone --branch v9.3.0 --depth=1 https://github.com/Kitware/VTK.git third_party/vtk
     cd third_party/vtk
     mkdir -p build-wasm && cd build-wasm
     emcmake cmake .. \
       -DBUILD_SHARED_LIBS=OFF \
       -DVTK_BUILD_TESTING=OFF \
       -DVTK_BUILD_EXAMPLES=OFF \
       -DVTK_BUILD_ALL_MODULES=OFF \
       -DVTK_ENABLE_WRAPPING=OFF \
       -DVTK_WRAP_PYTHON=OFF \
       -DVTK_WRAP_JAVA=OFF \
       -DVTK_BUILD_DOCUMENTATION=OFF \
       -DVTK_ENABLE_LOGGING=OFF \
       -DVTK_GROUP_ENABLE_StandAlone=DEFAULT \
       -DVTK_GROUP_ENABLE_Rendering=NO \
       -DVTK_GROUP_ENABLE_Imaging=NO \
       -DVTK_GROUP_ENABLE_Web=NO \
       -DVTK_GROUP_ENABLE_Views=NO \
       -DVTK_USE_MPI=OFF \
       -DVTK_MODULE_ENABLE_VTK_CommonCore=YES \
       -DVTK_MODULE_ENABLE_VTK_CommonDataModel=YES \
       -DVTK_MODULE_ENABLE_VTK_CommonExecutionModel=YES \
       -DVTK_MODULE_ENABLE_VTK_IOCore=YES \
       -DVTK_MODULE_ENABLE_VTK_IOXML=YES \
       -DVTK_MODULE_ENABLE_VTK_IOXMLParser=YES \
       -DVTK_MODULE_ENABLE_VTK_IOLegacy=YES \
       -DVTK_MODULE_ENABLE_VTK_FiltersGeometry=YES \
       -DVTK_MODULE_ENABLE_VTK_FiltersCore=YES \
       -DVTK_MODULE_ENABLE_VTK_RenderingCore=NO
     cmake --build . -j4
     cd ../../../..
     ```


3. **配置主项目**
   - 目录位置：项目根目录（即 `uvf-c/`）
   - 在项目根目录下执行：
     ```sh
     emcmake cmake -S . -B build-wasm -DVTK_DIR=$PWD/third_party/vtk/build-wasm/lib/cmake/vtk-9.3 -DCMAKE_BUILD_TYPE=Release -DUVF_BUILD_CLI=OFF
     ```


4. **编译主项目**
   - 目录位置：项目根目录（即 `uvf-c/`）
   - 在项目根目录下执行：
     ```sh
     cmake --build build-wasm -j4
     ```


5. **收集 wasm 产物**
  - 目录位置：项目根目录下的 `build-wasm/` 目录
  - 产物通常为 `build-wasm/uvf.wasm`、`build-wasm/uvf.js`，可拷贝到 `dist/` 目录用于发布或前端调用。

---

如需自动化，可将上述流程写入脚本或 CI/CD 流程。
