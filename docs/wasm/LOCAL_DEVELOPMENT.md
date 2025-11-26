# 本地 WASM 开发指南

本文档提供在本地构建和调试 UVF-C WebAssembly 模块的完整指南。

## 目录

- [环境准备](#环境准备)
- [快速开始](#快速开始)
- [手动构建](#手动构建)
- [增量构建](#增量构建)
- [调试技巧](#调试技巧)
- [常见问题](#常见问题)

---

## 环境准备

### 1. 安装 Emscripten SDK

Emscripten 是将 C++ 编译为 WebAssembly 的工具链。推荐使用版本 **4.0.13**（与 CI 保持一致）。

```bash
# 克隆 emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# 安装并激活指定版本
./emsdk install 4.0.13
./emsdk activate 4.0.13

# 激活环境变量（每次新终端都需要执行）
source emsdk_env.sh

# 验证安装
emcc --version
# 应显示类似: emcc (Emscripten gcc/clang-like replacement) 4.0.13
```

> **提示**: 建议将 `emsdk` 目录放在项目根目录或 `$HOME` 下，脚本会自动检测。

### 2. 系统依赖

| 依赖 | 版本要求 | 用途 |
|------|----------|------|
| CMake | >= 3.13 | 构建系统 |
| Git | 任意 | 克隆 VTK 源码 |
| Python 3 | 任意 | 开发服务器 |
| C++ 编译器 | C++17 | 构建 VTK |

macOS:

```bash
brew install cmake python3
```

Ubuntu/Debian:

```bash
sudo apt-get install cmake python3 build-essential
```

---

## 快速开始

使用一键开发脚本完成构建：

```bash
# 确保 emsdk 环境已激活
source ~/emsdk/emsdk_env.sh

# 一键构建
./scripts/dev-wasm.sh

# 构建并启动开发服务器
./scripts/dev-wasm.sh --serve
```

首次构建需要编译 VTK 依赖，大约需要 **20-40 分钟**。后续构建会自动复用已有的 VTK，仅需 1-2 分钟。

### 脚本选项

| 选项 | 说明 |
|------|------|
| `--serve` | 构建后启动本地开发服务器 (端口 8080) |
| `--port PORT` | 指定开发服务器端口 |
| `--clean` | 清理 `build-wasm/` 目录后重新构建 |
| `--emsdk PATH` | 指定 emsdk 根目录 |
| `--jobs N` | 并行编译任务数 |
| `--debug` | 构建 Debug 版本 |
| `--help` | 显示帮助信息 |

### 示例

```bash
# 清理重建并启动服务器
./scripts/dev-wasm.sh --clean --serve

# 使用 Debug 模式构建（便于调试）
./scripts/dev-wasm.sh --debug --serve

# 指定 emsdk 路径
./scripts/dev-wasm.sh --emsdk /opt/emsdk
```

---

## 手动构建

如果你需要更细粒度的控制，可以按以下步骤手动构建。

### 步骤 1: 激活 emsdk

```bash
source ~/emsdk/emsdk_env.sh
```

### 步骤 2: 构建 VTK (首次)

```bash
# 克隆 VTK
mkdir -p third_party
git clone --depth=1 --branch v9.3.0 https://github.com/Kitware/VTK.git third_party/vtk

# 配置 VTK
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
    -DVTK_SMP_IMPLEMENTATION_TYPE=Sequential \
    -DThreads_FOUND=TRUE \
    -DCMAKE_USE_PTHREADS_INIT=1 \
    -DCMAKE_HAVE_THREADS_LIBRARY=1 \
    -DCMAKE_THREAD_LIBS_INIT="-pthread" \
    -DVTK_MODULE_ENABLE_VTK_CommonCore=YES \
    -DVTK_MODULE_ENABLE_VTK_CommonDataModel=YES \
    -DVTK_MODULE_ENABLE_VTK_CommonExecutionModel=YES \
    -DVTK_MODULE_ENABLE_VTK_IOCore=YES \
    -DVTK_MODULE_ENABLE_VTK_IOXML=YES \
    -DVTK_MODULE_ENABLE_VTK_IOXMLParser=YES \
    -DVTK_MODULE_ENABLE_VTK_IOLegacy=YES \
    -DVTK_MODULE_ENABLE_VTK_FiltersGeometry=YES \
    -DVTK_MODULE_ENABLE_VTK_FiltersCore=YES \
    -DVTK_MODULE_ENABLE_VTK_IOGeometry=YES

# 构建 VTK
cmake --build . -j$(nproc)

# 返回项目根目录
cd ../../..
```

### 步骤 3: 构建项目

```bash
# 配置项目
emcmake cmake -S . -B build-wasm \
    -DVTK_DIR=$PWD/third_party/vtk/build-wasm/lib/cmake/vtk-9.3 \
    -DCMAKE_BUILD_TYPE=Release \
    -DUVF_BUILD_CLI=OFF

# 构建
cmake --build build-wasm -j$(nproc)
```

### 步骤 4: 收集产物并测试

```bash
# 复制产物
mkdir -p dist
cp build-wasm/uvf.js build-wasm/uvf.wasm dist/
cp index.html dist/

# 启动开发服务器
cd dist && python3 -m http.server 8080
```

访问 <http://localhost:8080> 测试转换功能。

---

## 增量构建

在开发过程中，你通常只需要重新编译项目代码，而不需要重新编译 VTK。

### 仅重新构建项目

```bash
# 确保 emsdk 环境已激活
source ~/emsdk/emsdk_env.sh

# 仅重新构建项目（跳过 VTK）
cmake --build build-wasm -j$(nproc)

# 复制新产物
cp build-wasm/uvf.js build-wasm/uvf.wasm dist/
```

或使用脚本：

```bash
./scripts/dev-wasm.sh  # 自动检测并跳过 VTK 构建
```

### 清理项目构建

```bash
# 仅清理项目构建（保留 VTK）
rm -rf build-wasm

# 或使用脚本
./scripts/dev-wasm.sh --clean
```

### 完全重新构建

如果需要更新 VTK 版本或解决依赖问题：

```bash
# 清理所有构建
rm -rf build-wasm
rm -rf third_party/vtk/build-wasm

# 重新构建
./scripts/dev-wasm.sh
```

---

## 调试技巧

### 1. 启用 Source Maps

构建 Debug 版本以获得更好的调试体验：

```bash
./scripts/dev-wasm.sh --debug --serve
```

在 Chrome DevTools 中：

1. 打开 **Sources** 面板
2. 在左侧文件树中找到 C++ 源文件
3. 设置断点进行调试

### 2. Console 调试

WASM 模块通过 Emscripten 提供的 `Module` 对象暴露 API：

```javascript
// 在浏览器控制台中
UVFModule().then(Module => {
    // 查看导出的函数
    console.log(Module._parse_vtp);
    console.log(Module._generate_uvf);

    // 查看虚拟文件系统
    console.log(Module.FS.readdir('/'));

    // 获取版本信息
    const version = Module.ccall('uvf_get_version', 'string', [], []);
    console.log('UVF Version:', version);

    // 获取最后一次操作的错误信息
    const error = Module.ccall('uvf_get_last_error', 'string', [], []);
    console.log('Last Error:', error);
});
```

### 3. 内存调试

如果遇到内存问题，可以在浏览器 DevTools 中：

1. **Memory 面板**: 拍摄堆快照，分析内存使用
2. **Performance 面板**: 记录性能轨迹，查看内存增长

Emscripten 提供的内存调试 API：

```javascript
UVFModule().then(Module => {
    // 查看当前内存使用
    console.log('HEAP size:', Module.HEAPU8.length);

    // 监控内存增长
    const initialSize = Module.HEAPU8.length;
    // ... 执行操作 ...
    console.log('Memory growth:', Module.HEAPU8.length - initialSize);
});
```

### 4. 虚拟文件系统调试

Emscripten 使用虚拟文件系统（MEMFS）：

```javascript
UVFModule().then(Module => {
    // 列出根目录文件
    console.log(Module.FS.readdir('/'));

    // 检查文件是否存在
    try {
        const stat = Module.FS.stat('/output/manifest.json');
        console.log('File size:', stat.size);
    } catch (e) {
        console.log('File not found');
    }

    // 读取文件内容
    const content = Module.FS.readFile('/output/manifest.json', { encoding: 'utf8' });
    console.log(JSON.parse(content));
});
```

### 5. C++ 端调试输出

在 C++ 代码中添加调试输出：

```cpp
#include <emscripten.h>

// 在 WASM 中使用 console.log
EM_ASM({
    console.log('Debug: points count =', $0);
}, point_count);

// 或使用 printf (会输出到控制台)
printf("Debug: processing file %s\n", filename);
```

---

## 常见问题

### Q: emcc 命令找不到

**A**: 确保已激活 emsdk 环境：

```bash
source ~/emsdk/emsdk_env.sh
```

建议将此行添加到 `~/.bashrc` 或 `~/.zshrc`。

### Q: wasm-ld not found in LLVM directory (Homebrew LLVM 冲突)

**A**: 如果安装了 Homebrew 的 LLVM，可能会与 emsdk 的 LLVM 冲突，出现类似错误：

```text
em++: error: linker binary not found in LLVM directory: /opt/homebrew/opt/llvm/bin/wasm-ld
```

解决方案：

1. **使用最新的 `dev-wasm.sh` 脚本**（已自动修复此问题）：

   ```bash
   ./scripts/dev-wasm.sh --serve
   ```

2. **手动修复**（如果仍有问题）：

   ```bash
   # 确保 emsdk 的 LLVM 路径优先
   export PATH="$HOME/Workspace/emsdk/upstream/bin:$PATH"

   # 或者取消 Homebrew LLVM 的 PATH 设置
   # 检查 ~/.zshrc 或 ~/.bashrc 中是否有类似:
   # export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
   # 如果有，在 emsdk 开发时临时注释掉
   ```

3. **验证使用的是正确的 LLVM**：

   ```bash
   which clang
   # 应该显示 emsdk 路径，如: ~/Workspace/emsdk/upstream/bin/clang
   ```

### Q: VTK 构建失败

**A**: 常见原因和解决方案：

1. **Threads 依赖错误** (`Could NOT find Threads`):

   这是 Emscripten 环境下的常见问题。确保使用最新版本的 `dev-wasm.sh` 脚本，它已包含必要的修复：

   ```bash
   # 清理旧的 VTK 构建
   rm -rf third_party/vtk/build-wasm

   # 重新构建
   ./scripts/dev-wasm.sh
   ```

   如果手动构建，需要添加以下 CMake 选项来模拟 Threads 支持：

   ```bash
   -DVTK_SMP_IMPLEMENTATION_TYPE=Sequential \
   -DThreads_FOUND=TRUE \
   -DCMAKE_USE_PTHREADS_INIT=1 \
   -DCMAKE_HAVE_THREADS_LIBRARY=1 \
   -DCMAKE_THREAD_LIBS_INIT="-pthread"
   ```

2. **内存不足**: VTK 构建需要较多内存，减少并行数：

   ```bash
   ./scripts/dev-wasm.sh --jobs 2
   ```

3. **网络问题**: VTK 克隆失败时，可手动下载：

   ```bash
   curl -L https://github.com/Kitware/VTK/archive/refs/tags/v9.3.0.tar.gz -o vtk.tar.gz
   tar -xzf vtk.tar.gz -C third_party
   mv third_party/VTK-9.3.0 third_party/vtk
   ```

4. **CMake 版本过低**: 确保 CMake >= 3.13

### Q: WASM 构建成功但运行报错

**A**: 常见问题：

1. **CORS 错误**: 必须通过 HTTP 服务器访问，不能直接打开 HTML 文件

   ```bash
   cd dist && python3 -m http.server 8080
   ```

2. **SharedArrayBuffer 错误**: 需要设置 COOP/COEP headers（简单开发环境通常不需要）

3. **内存不足**: 处理大文件时可能需要增加内存限制

### Q: 如何更新 VTK 版本?

**A**:

```bash
# 删除旧的 VTK
rm -rf third_party/vtk

# 重新克隆新版本
git clone --depth=1 --branch v9.4.0 https://github.com/Kitware/VTK.git third_party/vtk

# 重新构建
./scripts/dev-wasm.sh --clean
```

注意：更新 VTK 版本可能需要相应更新 `CMakeLists.txt` 中的路径。

### Q: 如何加快构建速度?

**A**:

- 使用更多并行任务：`--jobs 8`
- 使用 Release 模式（默认）
- 首次构建后，VTK 会被缓存，后续构建很快
- 考虑使用 ccache（如果可用）

### Q: 构建产物在哪里?

**A**:

- 项目构建目录: `build-wasm/`
- 可部署产物: `dist/`
  - `uvf.js` - JavaScript 胶水代码
  - `uvf.wasm` - WebAssembly 二进制
  - `index.html` - Demo 页面

---

## 参考资源

- [Emscripten 文档](https://emscripten.org/docs/)
- [WebAssembly 调试指南](https://developer.chrome.com/docs/devtools/wasm/)
- [VTK 官方文档](https://vtk.org/documentation/)
- [项目 GitHub Workflow](.github/workflows/build-wasm.yml) - CI 构建配置参考
