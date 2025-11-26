# local-wasm-development Specification

## Purpose

TBD - created by archiving change add-local-wasm-dev. Update Purpose after archive.

## Requirements

### Requirement: 本地 WASM 一键构建

开发者 SHALL 能够通过单一脚本命令完成本地 WASM 构建，无需手动执行多步操作。

#### Scenario: 首次构建

- **WHEN** 开发者在项目根目录执行 `./scripts/dev-wasm.sh`
- **AND** 本地未安装 emsdk
- **THEN** 脚本提示 emsdk 未找到并给出安装指引

#### Scenario: 正常构建

- **WHEN** 开发者已配置 emsdk 环境
- **AND** 执行 `./scripts/dev-wasm.sh`
- **THEN** 脚本自动检测并复用已有 VTK wasm 构建
- **AND** 构建项目生成 `uvf.js` 和 `uvf.wasm`
- **AND** 产物复制到 `dist/` 目录

#### Scenario: 启动开发服务器

- **WHEN** 开发者执行 `./scripts/dev-wasm.sh --serve`
- **THEN** 构建完成后自动启动 HTTP 服务器
- **AND** 输出访问地址（默认 <http://localhost:8080>）

### Requirement: 增量构建支持

开发者 SHALL 能够仅重新构建项目代码而跳过 VTK 依赖构建，以加快开发迭代速度。

#### Scenario: 仅构建项目

- **WHEN** VTK wasm 依赖已存在于 `third_party/vtk/build-wasm/`
- **AND** 开发者执行 `./scripts/dev-wasm.sh`
- **THEN** 脚本跳过 VTK 构建步骤
- **AND** 仅重新构建项目代码

#### Scenario: 强制完整重建

- **WHEN** 开发者执行 `./scripts/dev-wasm.sh --clean`
- **THEN** 清理 `build-wasm/` 目录
- **AND** 重新完整构建项目（不清理 VTK）

### Requirement: 本地开发文档

项目 SHALL 提供完整的本地 WASM 开发指南文档，涵盖环境准备、构建步骤和调试技巧。

#### Scenario: 文档可访问性

- **WHEN** 开发者查看 `docs/wasm/LOCAL_DEVELOPMENT.md`
- **THEN** 文档包含环境准备、快速开始、手动构建、调试技巧和常见问题章节

#### Scenario: 调试技巧

- **WHEN** 开发者需要调试 WASM 模块
- **THEN** 文档提供 Chrome DevTools Source Maps 配置说明
- **AND** 文档提供 Console API 调试方法
- **AND** 文档提供内存问题排查指引

### Requirement: README 入口

项目 README SHALL 提供本地 WASM 开发的快速入口链接。

#### Scenario: 快速导航

- **WHEN** 开发者查看项目 README
- **THEN** 能找到指向本地 WASM 开发文档的链接
- **AND** 能看到一键构建的快速命令示例
