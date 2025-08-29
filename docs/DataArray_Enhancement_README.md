# DataArray Information Enhancement

## 概述

已成功为 `generate_uvf` 方法添加了返回 DataArray 信息的功能，包括 `rangeMin` 和 `rangeMax` 等统计信息。

## 修改内容

### 1. 新增数据结构 (`vtp_to_uvf.h`)

```cpp
struct DataArrayInfo {
    string name;        // 数组名称
    int components;     // 组件数量
    size_t tuples;      // 元组数量  
    float rangeMin;     // 最小值
    float rangeMax;     // 最大值
    string dType;       // 数据类型
};
```

### 2. 方法签名更新

```cpp
// 新的方法签名（向后兼容）
bool generate_uvf(vtkPolyData* poly, const char* uvf_dir, vector<DataArrayInfo>* array_info = nullptr);
```

### 3. 功能特点

- **向后兼容**: 可以不传递 `array_info` 参数，行为与之前完全一致
- **详细信息**: 当传递 `array_info` 参数时，会收集每个 DataArray 的完整信息
- **自动计算范围**: 在处理数据时自动计算最小值和最大值
- **多组件支持**: 支持多组件数据数组

## 使用示例

### 获取 DataArray 信息
```cpp
vector<DataArrayInfo> arrayInfo;
if (generate_uvf(polydata, "output_dir", &arrayInfo)) {
    for (const auto& info : arrayInfo) {
        cout << "Array: " << info.name << endl;
        cout << "  Range: [" << info.rangeMin << ", " << info.rangeMax << "]" << endl;
        cout << "  Components: " << info.components << endl;
        cout << "  Tuples: " << info.tuples << endl;
    }
}
```

### 传统使用方式（仍然支持）
```cpp
// 不需要数组信息时的使用方式
if (generate_uvf(polydata, "output_dir")) {
    cout << "UVF generation successful!" << endl;
}
```

## 测试结果

使用 `star.vtp` 文件测试，成功检测到：
- `PressureCoefficient`: 范围 [-3.94517, 1.10592]
- `Pressuregradient`: 范围 [0.114848, 1.13553e+06]

每个数组都有 354,452 个数据点，都是单组件 float32 类型。
