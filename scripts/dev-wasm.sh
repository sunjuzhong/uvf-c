#!/usr/bin/env bash
#
# dev-wasm.sh - 本地 WASM 开发一键脚本
#
# 用法:
#   ./scripts/dev-wasm.sh [选项]
#
# 选项:
#   --serve       构建后启动本地开发服务器 (默认端口 8080)
#   --port PORT   指定开发服务器端口 (默认 8080)
#   --clean       清理 build-wasm 目录后重新构建
#   --emsdk PATH  指定 emsdk 根目录
#   --jobs N      并行编译任务数 (默认自动检测)
#   --debug       构建 Debug 版本 (默认 Release)
#   -h, --help    显示帮助信息
#
set -euo pipefail

# ========== 配置 ==========
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

EMSDK=""
BUILD_TYPE="Release"
SERVE=0
PORT=8080
CLEAN=0
JOBS=""
VTK_VERSION="v9.3.0"

# ========== 颜色输出 ==========
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# ========== 帮助信息 ==========
show_help() {
    cat << 'EOF'
本地 WASM 开发一键脚本

用法:
  ./scripts/dev-wasm.sh [选项]

选项:
  --serve       构建后启动本地开发服务器 (默认端口 8080)
  --port PORT   指定开发服务器端口 (默认 8080)
  --clean       清理 build-wasm 目录后重新构建
  --emsdk PATH  指定 emsdk 根目录
  --jobs N      并行编译任务数 (默认自动检测)
  --debug       构建 Debug 版本 (默认 Release)
  -h, --help    显示帮助信息

示例:
  # 快速构建
  ./scripts/dev-wasm.sh

  # 构建并启动开发服务器
  ./scripts/dev-wasm.sh --serve

  # 清理重建并启动服务器
  ./scripts/dev-wasm.sh --clean --serve

  # 使用指定 emsdk 路径
  ./scripts/dev-wasm.sh --emsdk ~/emsdk

环境变量:
  EMSDK        emsdk 根目录路径
  VTK_DIR      已构建的 VTK wasm 路径 (跳过 VTK 构建)
EOF
}

# ========== 参数解析 ==========
while [[ $# -gt 0 ]]; do
    case "$1" in
        --serve)
            SERVE=1
            shift
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        --emsdk)
            EMSDK="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "未知参数: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

# ========== 检测并行任务数 ==========
if [[ -z "$JOBS" ]]; then
    if command -v nproc &>/dev/null; then
        JOBS=$(nproc)
    elif command -v sysctl &>/dev/null; then
        JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    else
        JOBS=4
    fi
fi

# ========== 进入项目目录 ==========
cd "$PROJECT_ROOT"
log_info "项目目录: $PROJECT_ROOT"

# ========== 检测 emsdk 环境 ==========
setup_emsdk() {
    # 尝试从参数或环境变量加载
    local emsdk_root="${EMSDK:-${EMSDK_ROOT:-}}"

    # 常见路径检测
    local search_paths=(
        "$emsdk_root"
        "$PROJECT_ROOT/emsdk"
        "$HOME/emsdk"
        "$HOME/Workspace/emsdk"
        "/opt/emsdk"
    )

    local found_emsdk=""
    for path in "${search_paths[@]}"; do
        if [[ -n "$path" && -f "$path/emsdk_env.sh" ]]; then
            found_emsdk="$path"
            break
        fi
    done

    if [[ -z "$found_emsdk" ]]; then
        # 未找到 emsdk
        log_error "未找到 emsdk 环境!"
        echo ""
        echo "请按以下步骤安装 emsdk:"
        echo ""
        echo "  git clone https://github.com/emscripten-core/emsdk.git"
        echo "  cd emsdk"
        echo "  ./emsdk install 4.0.13"
        echo "  ./emsdk activate 4.0.13"
        echo "  source emsdk_env.sh"
        echo ""
        echo "或使用 --emsdk 参数指定 emsdk 路径:"
        echo "  ./scripts/dev-wasm.sh --emsdk ~/emsdk"
        echo ""
        exit 1
    fi

    log_info "加载 emsdk 环境: $found_emsdk"
    source "$found_emsdk/emsdk_env.sh"

    # 确保 emsdk 的 LLVM 优先于系统 LLVM (如 Homebrew)
    # 这解决了 Homebrew LLVM 导致的 "wasm-ld not found" 错误
    if [[ -d "$found_emsdk/upstream/bin" ]]; then
        export PATH="$found_emsdk/upstream/bin:$PATH"
        log_info "设置 LLVM 路径: $found_emsdk/upstream/bin"
    fi

    # 验证 emcc 可用
    if ! command -v emcc &>/dev/null; then
        log_error "emsdk 激活后 emcc 仍不可用"
        exit 1
    fi

    log_ok "emsdk 环境已激活: $(emcc --version | head -1)"
}

# ========== 检测/构建 VTK wasm ==========
setup_vtk() {
    # 如果已设置 VTK_DIR 且有效，跳过
    if [[ -n "${VTK_DIR:-}" && -f "$VTK_DIR/VTKConfig.cmake" ]]; then
        log_ok "使用已有 VTK: $VTK_DIR"
        return 0
    fi

    local vtk_src="$PROJECT_ROOT/third_party/vtk"
    local vtk_build="$vtk_src/build-wasm"

    # 检测 VTK 版本目录
    local vtk_mm="${VTK_VERSION#v}"      # v9.3.0 -> 9.3.0
    vtk_mm="${vtk_mm%.*}"                # 9.3.0 -> 9.3
    local vtk_cmake_dir="$vtk_build/lib/cmake/vtk-$vtk_mm"

    if [[ -f "$vtk_cmake_dir/VTKConfig.cmake" ]]; then
        log_ok "复用已有 VTK wasm 构建: $vtk_cmake_dir"
        export VTK_DIR="$vtk_cmake_dir"
        return 0
    fi

    log_warn "未找到 VTK wasm 构建，将自动构建..."
    log_info "这可能需要 20-40 分钟，请耐心等待"
    echo ""

    # 克隆 VTK
    if [[ ! -d "$vtk_src" ]]; then
        log_info "克隆 VTK ${VTK_VERSION}..."
        mkdir -p third_party
        git clone --depth=1 --branch "${VTK_VERSION}" https://github.com/Kitware/VTK.git "$vtk_src"
    fi

    # 构建 VTK
    log_info "配置 VTK for wasm..."
    mkdir -p "$vtk_build"
    pushd "$vtk_build" >/dev/null

    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
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
        -DVTK_SMP_IMPLEMENTATION_TYPE=Sequential \
        -DThreads_FOUND=TRUE \
        -DCMAKE_USE_PTHREADS_INIT=1 \
        -DCMAKE_HAVE_THREADS_LIBRARY=1 \
        -DCMAKE_THREAD_LIBS_INIT="-pthread" \
        -DVTK_MODULE_ENABLE_VTK_IOGeometry=YES \
        -DVTK_MODULE_ENABLE_VTK_FiltersHybrid=YES \
        -DVTK_MODULE_ENABLE_VTK_ImagingSources=YES \
        -DVTK_MODULE_ENABLE_VTK_ImagingCore=YES \
        -DVTK_MODULE_ENABLE_VTK_CommonCore=YES \
        -DVTK_MODULE_ENABLE_VTK_CommonDataModel=YES \
        -DVTK_MODULE_ENABLE_VTK_CommonExecutionModel=YES \
        -DVTK_MODULE_ENABLE_VTK_IOCore=YES \
        -DVTK_MODULE_ENABLE_VTK_IOXML=YES \
        -DVTK_MODULE_ENABLE_VTK_IOXMLParser=YES \
        -DVTK_MODULE_ENABLE_VTK_IOLegacy=YES \
        -DVTK_MODULE_ENABLE_VTK_FiltersGeometry=YES \
        -DVTK_MODULE_ENABLE_VTK_FiltersCore=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingCore=YES \
        -DVTK_MODULE_ENABLE_VTK_IOCellGrid=NO \
        -DVTK_MODULE_ENABLE_VTK_FiltersCellGrid=NO \
        -DCMAKE_INSTALL_PREFIX="$vtk_build/install"

    log_info "构建 VTK (并行度: $JOBS)..."
    cmake --build . -j "$JOBS"
    cmake --install .

    popd >/dev/null

    export VTK_DIR="$vtk_cmake_dir"
    log_ok "VTK wasm 构建完成: $VTK_DIR"
}

# ========== 构建项目 ==========
build_project() {
    local build_dir="$PROJECT_ROOT/build-wasm"

    # 清理构建目录
    if [[ $CLEAN -eq 1 && -d "$build_dir" ]]; then
        log_info "清理构建目录: $build_dir"
        rm -rf "$build_dir"
    fi

    mkdir -p "$build_dir"

    log_info "配置项目 (${BUILD_TYPE})..."
    emcmake cmake -S "$PROJECT_ROOT" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DVTK_DIR="$VTK_DIR" \
        -DUVF_BUILD_CLI=OFF

    log_info "构建项目 (并行度: $JOBS)..."
    cmake --build "$build_dir" -j "$JOBS"

    log_ok "项目构建完成"
}

# ========== 收集产物 ==========
collect_artifacts() {
    local dist_dir="$PROJECT_ROOT/dist"
    mkdir -p "$dist_dir"

    log_info "复制产物到 dist/..."

    cp "$PROJECT_ROOT/build-wasm/uvf.js" "$dist_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/build-wasm/uvf.wasm" "$dist_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/index.html" "$dist_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/demo.js" "$dist_dir/" 2>/dev/null || true

    log_ok "产物已复制到 dist/:"
    ls -lh "$dist_dir"/*.{js,wasm,html} 2>/dev/null || true
}

# ========== 启动开发服务器 ==========
start_server() {
    local dist_dir="$PROJECT_ROOT/dist"

    echo ""
    log_info "启动开发服务器..."
    echo ""
    echo "========================================"
    echo "  访问地址: http://localhost:$PORT"
    echo "  按 Ctrl+C 停止服务器"
    echo "========================================"
    echo ""

    cd "$dist_dir"

    if command -v python3 &>/dev/null; then
        python3 -m http.server "$PORT"
    elif command -v python &>/dev/null; then
        python -m SimpleHTTPServer "$PORT"
    else
        log_error "未找到 Python，无法启动 HTTP 服务器"
        echo "请手动运行: cd dist && npx serve -p $PORT"
        exit 1
    fi
}

# ========== 主流程 ==========
main() {
    echo ""
    echo "========================================"
    echo "  UVF-C 本地 WASM 开发脚本"
    echo "========================================"
    echo ""

    setup_emsdk
    setup_vtk
    build_project
    collect_artifacts

    echo ""
    log_ok "构建完成!"
    echo ""
    echo "产物位置: $PROJECT_ROOT/dist/"
    echo "  - uvf.js"
    echo "  - uvf.wasm"
    echo "  - index.html"
    echo ""

    if [[ $SERVE -eq 1 ]]; then
        start_server
    else
        echo "提示: 使用 --serve 可自动启动开发服务器"
        echo "  ./scripts/dev-wasm.sh --serve"
        echo ""
    fi
}

main

