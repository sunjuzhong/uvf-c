# Project Context

## Purpose

UVF-C 是一个高性能的 VTK 到 UVF（Universal Volume Format）转换器，使用 C++ 编写。项目的主要目标是：

- 将 VTK/VTP 科学可视化文件格式转换为 UVF 格式
- 支持多种输入格式：VTP (XML)、VTK (legacy)、结构化网格
- 提供原生 C++ 和 WebAssembly 构建，支持浏览器端处理
- 自动检测和分类几何类型（表面、切片、流线）
- 提取 VTK 数据数组的详细元数据（范围、类型、组件数）

## Tech Stack

- **语言**: C++17
- **构建系统**: CMake >= 3.13
- **核心依赖**: VTK (CommonCore, CommonDataModel, IOCore, IOXML, IOLegacy, FiltersGeometry)
- **JSON 处理**: nlohmann/json (可选，通过 third_party 引入)
- **WASM 工具链**: Emscripten SDK (3.1.64+)

### 构建目标

| 目标 | 类型 | 描述 |
|------|------|------|
| `libuvf.a` | 静态库 | 核心转换库 |
| `uvf_cli` | 可执行文件 | 命令行工具 |
| `uvf.js` / `uvf.wasm` | WASM 模块 | Web 端使用 |
| `uvf_tests` | 测试 | 几何分类测试 |
| `uvf_file_tests` | 测试 | 文件格式测试 |

## Project Conventions

### Code Style

- **命名规范**:
  - 函数和变量：`snake_case`（如 `parse_vtp_file`, `generate_uvf`）
  - 结构体/类：`PascalCase`（如 `DataArrayInfo`, `UVFOffsets`）
  - 常量/宏：`UPPER_SNAKE_CASE`

- **头文件**:
  - 使用 `#pragma once` 作为头文件保护
  - 公共 API 使用 Doxygen 风格注释

- **类型使用**:
  - 使用 `using` 声明简化常用类型：`using std::vector`, `using std::string`, `using std::map`
  - VTK 对象使用 `vtkSmartPointer<T>` 智能指针管理

- **注释语言**: 英文为主，文档可中文

### Architecture Patterns

```text
src/
├── vtp_to_uvf.{h,cpp}         # 核心转换引擎 - 解析 VTK 并生成 UVF
├── vtk_structured_parser.{h,cpp} # 结构化网格解析器
├── multi_file_parser.{h,cpp}    # 批量文件处理
├── id_utils.{h,cpp}             # ID 生成工具
├── uvf_c_api.{h,cpp}            # C API 封装层（供 WASM/跨语言调用）
└── main.cpp                     # CLI 入口
```

- **分层设计**:
  - 核心层：`vtp_to_uvf.cpp` 实现 VTK 数据解析和 UVF 生成
  - API 层：`uvf_c_api.cpp` 提供 C 接口，便于 Emscripten 导出和跨语言调用
  - 应用层：`main.cpp` 提供命令行界面

- **数据流**:

```text
VTK/VTP File → parse_vtp_file() → vtkPolyData → generate_uvf() → UVF Directory
                                                                  ├── manifest.json
                                                                  └── *.bin
```

### Testing Strategy

- **测试框架**: 手动断言测试（无外部测试框架依赖）
- **测试组织**:
  - `test_geom_kind.cpp`: 几何类型分类测试（streamline/slice/surface）
  - `test_file_inputs.cpp`: 文件格式输入测试
  - `test_data_array_info.cpp`: 数据数组元数据测试

- **测试数据**: 位于 `tests/data/` 目录
- **运行测试**:

```bash
cd build
make test
# 或单独运行
./uvf_tests
./uvf_file_tests
```

### Git Workflow

- 主分支用于稳定发布
- 功能开发使用特性分支
- 提交信息使用清晰的描述性语言

## Domain Context

### VTK 文件格式

- **VTP (XML PolyData)**: 现代 XML 格式，支持压缩和 DataArray 元数据
- **VTK (Legacy)**: 传统 ASCII/Binary 格式
- **结构化网格**: 规则网格数据（如体数据切片）

### 几何类型分类 (GeomKind)

| 类型 | 描述 | 检测条件 |
|------|------|----------|
| `streamline` | 流线 | 仅包含线段单元 |
| `slice` | 切片 | 多边形在平面上（Z 变化很小） |
| `surface` | 表面 | 默认，包含非平面多边形 |

### UVF 输出格式

- `manifest.json`: 元数据文件，包含几何类型、数据数组信息、偏移量等
- `*.bin`: 二进制数据文件，存储顶点、索引、标量数据

## Important Constraints

- **C++17 必需**: 使用 `<filesystem>` 等特性
- **VTK 版本兼容**: 针对 VTK 9.x 开发，WASM 构建使用 v9.3.0
- **WASM 限制**:
  - 必须使用 Emscripten 预编译的 VTK
  - 不支持原生文件系统，使用 Emscripten 虚拟文件系统
- **静态库**: 原生构建生成静态库 `libuvf.a`

## External Dependencies

### VTK Modules (必需)

| 模块 | 用途 |
|------|------|
| `VTK::CommonCore` | 基础数据类型 |
| `VTK::CommonDataModel` | 数据模型（PolyData 等） |
| `VTK::IOXML` | XML 格式读取 |
| `VTK::IOLegacy` | Legacy 格式读取 |
| `VTK::FiltersGeometry` | 几何过滤器 |

### 可选依赖

- **nlohmann/json**: JSON 序列化（通过 `third_party/nlohmann_json` 引入）
- **Emscripten SDK**: WASM 构建所需
