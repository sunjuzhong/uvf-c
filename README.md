# uvf-c: VTP→UVF WASM 项目模板

## 依赖
- VTK (C++)
- CMake >= 3.13
- Emscripten (编译为 WASM)

## 构建（本地 C++）
```sh
mkdir build && cd build
cmake ..
make
```

## 构建（Emscripten WASM）
```sh
source /path/to/emsdk/emsdk_env.sh
mkdir build-wasm && cd build-wasm
emcmake cmake ..
cmake --build . -j
```

生成的 `uvf.wasm` 和 `uvf.js` 可用于 Web 前端。

## 前端 JS 调用示例
```js
import UVFModule from './uvf.js';

UVFModule().then(Module => {
  // 假设已通过 fetch/FS 等方式将 vtp 文件加载到虚拟文件系统
  const vtpPath = '/input.vtp';
  const uvfPath = '/output.uvf';
  // 调用 WASM 导出的 C 接口
  const ok = Module._generate_uvf(vtpPath, uvfPath);
  if (ok) {
    // 读取生成的 UVF 文件
    const uvfData = Module.FS_readFile(uvfPath);
    // ...处理 uvfData
  }
});
```

### 原生 vs WASM
- 原生：生成 `libuvf.a` + `uvf_cli`
- WASM：生成 `uvf.js` (glue) + `uvf.wasm`，通过 `UVFModule()` 异步加载

### Demo 运行步骤
1. 构建 wasm (见上)
2. 将 `uvf.js` 与 `uvf.wasm` 放在 `index.html` 同目录
3. 启动静态服务器（避免浏览器 file:// 限制）
```sh
python3 -m http.server 8080
```
4. 浏览器访问 http://localhost:8080 ，选择 .vtp 转换

若需要 Worker 模式，可把 import 改到 worker 内并通过 postMessage 传递 ArrayBuffer。

## 说明
- 你可以在 `src/vtp_to_uvf.cpp` 实现更复杂的 VTP 解析和 UVF 生成逻辑。
- C 接口在 `src/uvf_c_api.cpp`，方便 Emscripten 导出。
- CMakeLists.txt 已配置 VTK、Emscripten、WASM 导出参数。
