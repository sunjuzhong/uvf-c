# stl-conversion Specification

## Purpose
TBD - created by archiving change add-stl-to-uvf-conversion. Update Purpose after archive.
## Requirements
### Requirement: STL File Parsing

系统 SHALL 支持解析 STL（Stereolithography）文件格式，包括 ASCII 和 Binary 两种变体。

#### Scenario: ASCII STL 文件解析成功

- **GIVEN** 一个有效的 ASCII STL 文件
- **WHEN** 调用 `parse_stl_file()` 或 `parse_vtp_file()` 函数
- **THEN** 返回包含顶点和三角面数据的 `vtkPolyData` 对象

#### Scenario: Binary STL 文件解析成功

- **GIVEN** 一个有效的 Binary STL 文件
- **WHEN** 调用 `parse_stl_file()` 或 `parse_vtp_file()` 函数
- **THEN** 返回包含顶点和三角面数据的 `vtkPolyData` 对象

#### Scenario: 无效 STL 文件处理

- **GIVEN** 一个无效或损坏的 STL 文件
- **WHEN** 调用解析函数
- **THEN** 返回 `nullptr` 并设置相应错误信息

### Requirement: STL 文件扩展名自动检测

系统 SHALL 通过文件扩展名 `.stl`（不区分大小写）自动识别 STL 文件并路由到正确的解析器。

#### Scenario: 自动检测 .stl 扩展名

- **GIVEN** 输入文件路径以 `.stl` 或 `.STL` 结尾
- **WHEN** 调用 `parse_vtp_file()` 函数
- **THEN** 自动使用 STL 解析器处理文件

### Requirement: STL 到 UVF 转换

系统 SHALL 将 STL 文件转换为 UVF 格式，生成 `manifest.json` 和对应的二进制数据文件。

#### Scenario: STL 转换生成正确的 UVF 结构

- **GIVEN** 一个有效的 STL 文件
- **WHEN** 执行转换操作
- **THEN** 生成包含以下内容的 UVF 目录：
  - `manifest.json` 包含几何元数据
  - `*.bin` 文件包含索引和顶点数据
  - 几何类型标记为 `surface`

#### Scenario: 多 solid STL 文件转换

- **GIVEN** 一个包含多个 `solid` 块的 STL 文件
- **WHEN** 执行转换操作
- **THEN** 每个 solid 生成独立的 Face 对象，并在 manifest 中正确引用

### Requirement: CLI 支持 STL 输入

命令行工具 SHALL 支持 STL 文件作为输入。

#### Scenario: CLI 处理 STL 文件

- **GIVEN** 用户通过 CLI 指定 STL 文件路径
- **WHEN** 执行 `uvf_cli input.stl output_dir`
- **THEN** 成功转换并输出 UVF 格式

### Requirement: C API 支持 STL 处理

C API SHALL 提供 STL 文件处理能力，与现有 VTP 处理接口保持一致。

#### Scenario: C API 生成 UVF 从 STL

- **GIVEN** 一个有效的 STL 文件路径
- **WHEN** 调用 `generate_uvf()` 函数
- **THEN** 成功生成 UVF 输出目录

