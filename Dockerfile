###########################
# Stage 1: Build minimal VTK for wasm
###########################
FROM emscripten/emsdk:4.0.13 AS vtk
ARG VTK_TAG=v9.3.0
ARG VTK_PARALLEL_JOBS
# VTK_PARALLEL_JOBS: optional override; fallback to nproc at build time
ENV VTK_TAG=${VTK_TAG} VTK_PARALLEL_JOBS=${VTK_PARALLEL_JOBS}
WORKDIR /opt
RUN git clone --depth=1 --branch ${VTK_TAG} https://github.com/Kitware/VTK.git vtk
WORKDIR /opt/vtk/build-wasm
RUN emcmake cmake .. \
  -DBUILD_SHARED_LIBS=OFF \
  -DVTK_BUILD_TESTING=OFF \
  -DVTK_BUILD_EXAMPLES=OFF \
  -DVTK_BUILD_ALL_MODULES=OFF \
  -DVTK_ENABLE_WRAPPING=OFF \
  -DVTK_WRAP_PYTHON=OFF \
  -DVTK_WRAP_JAVA=OFF \
  -DVTK_BUILD_DOCUMENTATION=OFF \
  -DVTK_ENABLE_LOGGING=OFF \
  -DVTK_USE_64BIT_IDS=ON \
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
  -DVTK_MODULE_ENABLE_VTK_RenderingCore=NO \
  -DVTK_MODULE_ENABLE_VTK_IOCellGrid=NO \
  -DVTK_MODULE_ENABLE_VTK_FiltersCellGrid=NO
RUN : "${VTK_PARALLEL_JOBS:=$(nproc)}" && echo "[info] Building VTK with ${VTK_PARALLEL_JOBS} jobs" && cmake --build . -j "${VTK_PARALLEL_JOBS}"

###########################
# Stage 2: Build project (wasm only artifacts)
###########################
FROM emscripten/emsdk:4.0.13 AS build
ARG BUILD_TYPE=Release
ARG PARALLEL_JOBS
ENV BUILD_TYPE=${BUILD_TYPE} PARALLEL_JOBS=${PARALLEL_JOBS}
WORKDIR /workspace

# Copy only files needed for CMake configuration first (improves layer cache)
COPY CMakeLists.txt ./
COPY src ./src
COPY third_party/nlohmann_json ./third_party/nlohmann_json
COPY index.html ./

# Bring in built VTK
COPY --from=vtk /opt/vtk/build-wasm /opt/vtk/build-wasm
ENV VTK_DIR=/opt/vtk/build-wasm/lib/cmake/vtk-9.3

# Clean any prior directory (defensive) then configure & build
RUN rm -rf build-wasm \
  && : "${PARALLEL_JOBS:=$(nproc)}" && echo "[info] Building project (${BUILD_TYPE}) with ${PARALLEL_JOBS} jobs" \
  && emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DVTK_DIR=${VTK_DIR} -DUVF_BUILD_CLI=OFF \
  && cmake --build build-wasm -j "${PARALLEL_JOBS}" \
  && echo "[info] Build complete. Artifacts in build-wasm/ (uvf.wasm, uvf.js)"

# Default command: list wasm build outputs
CMD ["/bin/sh","-c","ls -lh build-wasm/uvf.* 2>/dev/null || ls -lh build-wasm"]