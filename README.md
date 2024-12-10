# 中国行政区划数据管理与查询系统

基于树结构实现的中国行政区划数据管理系统，支持完整的行政区划层级关系查询。

### 性能
- 加载CSV文件 < 0.1 秒
- 构建树结构 < 0.15 秒
- 查询 < 0.01 秒

### 主要功能

- 支持代码精确查询和名称模糊查询
- 显示完整的行政区划层级关系
- 支持扩展数据（房价、就业率等）
- 基于树结构的高效存储和查询
- 使用二分查找及深度优先搜索加快查询速度<br>
<br>
## 数据格式

### CSV文件格式
```csv
code,name,level,parent_code,type[,avg_house_price,employment_rate](optional)

# 示例
# 110000000000,北京市,1,0,0,66946,96%
```
### CSV文件名
```bash
area_data.csv
```
你也可以使用其他文件名，并且修改 `loadRegionsFromCSV` 函数中的文件名
```c
// line 533
int size = loadRegionsFromCSV(regions, "your_file_name.csv");
```
### 行政级别
- 0级：国家级
- 1级：省级（省、直辖市、自治区等）
- 2级：地级（地级市、地区、自治州等）
- 3级：县级（市辖区、县级市、县等）
- 4级：乡级（街道、镇、乡等）
- 5级：村级（居委会、村委会等）<br>
<br>

## 使用方法（项目根目录下）

### Windows 平台
1. 在源代码头部添加以下内容：
```c
#include <windows.h>
```

2. 在 main 函数添加以下内容：
```c
SetConsoleOutputCP(CP_UTF8);
SetConsoleCP(CP_UTF8);
```

2. 编译(确保已安装 gcc)
```bash
gcc Administrative_division.c -o Administrative_division.exe
```

3. 运行
```bash
./Administrative_division.exe
```

### Linux/macOS 平台
```bash
# 使用 gcc
gcc Administrative_division.c -o Administrative_division
# 或使用 clang(macOS)
clang Administrative_division.c -o Administrative_division
```

### 运行
```bash
./Administrative_division
```

### 查询示例
```bash
1. 按代码查询：110000000000（北京市）
2. 按名称查询：北京（支持模糊匹配）
```
<br>

## 自定义名称查询结果数量
- 有多个查询结果时，默认显示前5条
- 通过修改 `findByNameRecursive` 函数中的 `max_results` 变量来修改显示结果数量
```c
if (root == NULL || *found >= max_results) return; // line 287

if (*found >= max_results) {
    printf("\n结果过多，仅显示前max_results条...\n"); // line 292
}
```
<br>

## 系统要求
- C99标准编译器
- UTF-8编码支持
- Windows 平台需要设置控制台为 UTF-8 编码（代码中已添加相关代码，无需手动设置）

## 平台特定说明

### Windows
- 需要包含 `windows.h` 头文件
- 需要设置控制台代码页为 UTF-8
- 建议使用支持 UTF-8 的终端（如 Windows Terminal）

### Linux/macOS
- 默认支持 UTF-8 编码
- 无需额外配置

## 注意事项
- 名称查询限制显示前5条匹配结果（修改方法见上文）
- CSV数据文件需为UTF-8编码
- Windows 平台下如显示乱码，请确保环境已正确设置 UTF-8 编码

## 数据来源
基于 [adyliu/china_area](https://github.com/adyliu/china_area) 的数据进行开发。