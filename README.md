# 中国行政区划数据管理与查询系统

基于树结构实现的中国行政区划数据管理系统，支持完整的行政区划层级关系查询。

## 主要功能

- 支持代码精确查询和名称模糊查询
- 显示完整的行政区划层级关系
- 支持扩展数据（房价、就业率等）
- 基于树结构的高效存储和查询

## 数据格式

### CSV文件格式
```csv
code,name,level,parent_code,type[,avg_house_price,employment_rate]
```

### 行政级别
- 0级：国家级
- 1级：省级（省、直辖市、自治区等）
- 2级：地级（地级市、地区、自治州等）
- 3级：县级（市辖区、县级市、县等）
- 4级：乡级（街道、镇、乡等）
- 5级：村级（居委会、村委会等）

## 使用方法

### 编译
```bash
gcc Administrative_division_new.c -o Administrative_division_new
```

### 运行
```bash
./Administrative_division_new
```

### 查询示例
```
1. 按代码查询：110000000000（北京市）
2. 按名称查询：北京（支持模糊匹配）
```

## 系统要求
- C99标准编译器
- UTF-8编码支持

## 注意事项
- 名称查询限制显示前5条匹配结果
- 数据文件需为UTF-8编码

## 数据来源
基于 [adyliu/china_area](https://github.com/adyliu/china_area) 的数据进行开发。