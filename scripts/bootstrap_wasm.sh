#!/usr/bin/env bash
set -euo pipefail

# bootstrap_wasm.sh
# 一键完成：检测/加载 emsdk -> 获取或复用 VTK wasm 版本 -> 构建本项目 uvf.js/uvf.wasm -> 输出 dist/
# 用法: scripts/bootstrap_wasm.sh [--emsdk ~/emsdk] [--vtk-src /path/to/vtk] [--build-type Release] [--jobs N]
# 可选：预先 export VTK_DIR 指向已构建好的 wasm 版 VTK，脚本将跳过 VTK 构建。

EMSDK=""
VTK_SRC=""
BUILD_TYPE="Release"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
VTK_VERSION="v9.3.0"
FULL_VTK=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --emsdk) EMSDK="$2"; shift 2;;
    --vtk-src) VTK_SRC="$2"; shift 2;;
    --build-type) BUILD_TYPE="$2"; shift 2;;
    --vtk-version) VTK_VERSION="$2"; shift 2;;
  --jobs) JOBS="$2"; shift 2;;
  --full-vtk) FULL_VTK=1; shift 1;;
    -h|--help)
  echo "Usage: $0 [--emsdk <emsdk_root>] [--vtk-src <vtk_git_dir>] [--vtk-version <tag>] [--build-type <Release|Debug>] [--jobs N] [--full-vtk]"; exit 0;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

log(){ echo -e "[bootstrap] $*"; }

if ! command -v emcc >/dev/null 2>&1; then
  if [[ -n "$EMSDK" ]]; then
    log "Sourcing emsdk: $EMSDK"; source "$EMSDK/emsdk_env.sh"
  else
    echo "emcc 不存在，请用 --emsdk 指定 emsdk 根目录或先手动 source emsdk_env.sh"; exit 1
  fi
fi

if [[ -z "${VTK_DIR:-}" ]]; then
  if [[ -z "$VTK_SRC" ]]; then
    mkdir -p third_party
    if [[ ! -d third_party/vtk ]]; then
      log "Cloning VTK (depth=1) ..."
        log "Cloning VTK (depth=1) from GitLab tag=${VTK_VERSION} ..."
        if ! git clone --depth=1 --branch "${VTK_VERSION}" https://gitlab.kitware.com/vtk/vtk.git third_party/vtk 2>/dev/null; then
          log "GitLab clone failed, trying GitHub mirror ..."
          if ! git clone --depth=1 --branch "${VTK_VERSION}" https://github.com/Kitware/VTK.git third_party/vtk 2>/dev/null; then
            log "Mirror clone failed, attempting archive download ..."
            mkdir -p third_party/vtk
            curl -L "https://github.com/Kitware/VTK/archive/refs/tags/${VTK_VERSION}.tar.gz" -o /tmp/vtk.tar.gz || { echo "Download VTK archive failed"; exit 1; }
            tar -xzf /tmp/vtk.tar.gz -C third_party
            mv "third_party/VTK-${VTK_VERSION#v}" third_party/vtk
          fi
        fi
    fi
    VTK_SRC=third_party/vtk
  fi
  VTK_BUILD="$VTK_SRC/build-wasm"
  VTK_MAJOR_MINOR="${VTK_VERSION#v}" # e.g. 9.4.2 -> 9.4.2
  VTK_MM_DIR="vtk-${VTK_MAJOR_MINOR%.*}" # 9.4.2 -> vtk-9.4
  if [[ ! -f "$VTK_BUILD/lib/cmake/${VTK_MM_DIR}/VTKConfig.cmake" ]]; then
    log "Configuring VTK for wasm (最小模块, FULL_VTK=$FULL_VTK)..."
    mkdir -p "$VTK_BUILD"; pushd "$VTK_BUILD" >/dev/null
    set +e
    if [[ $FULL_VTK -eq 1 ]]; then
      log "使用 --full-vtk: 启用 FiltersCore"
      emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
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
  -DVTK_MODULE_ENABLE_VTK_FiltersCore=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingCore=NO
    else
      emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
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
  -DVTK_MODULE_ENABLE_VTK_FiltersCore=NO \
        -DVTK_MODULE_ENABLE_VTK_RenderingCore=NO
      if [[ $? -ne 0 ]]; then
        log "最小配置失败，添加 FiltersCore 重试"
        rm -f CMakeCache.txt
        emcmake cmake .. \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
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
          -DVTK_MODULE_ENABLE_VTK_FiltersCore=YES \
          -DVTK_MODULE_ENABLE_VTK_RenderingCore=NO || { echo "VTK configure still failed"; exit 1; }
      fi
    fi
    set -e
    cmake --build . -j "$JOBS"
    popd >/dev/null
  else
    log "复用已存在的 VTK wasm 构建"
  fi
  export VTK_DIR="$VTK_BUILD/lib/cmake/${VTK_MM_DIR}"
fi

log "VTK_DIR=$VTK_DIR"

PROJECT_BUILD="build-wasm"
mkdir -p "$PROJECT_BUILD"; pushd "$PROJECT_BUILD" >/dev/null
log "Configuring project ..."
emcmake cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DVTK_DIR="$VTK_DIR"
log "Building project ..."
cmake --build . -j "$JOBS"
popd >/dev/null

mkdir -p dist
cp build-wasm/uvf.wasm build-wasm/uvf.js dist/
cp index.html demo.js dist/

log "完成: dist/ 下已有 uvf.js, uvf.wasm, index.html, demo.js"
log "可运行: (cd dist && python3 -m http.server 8080) 然后访问 http://localhost:8080"