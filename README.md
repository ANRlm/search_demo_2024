# Search Demo 2024

## 项目简介 (Project Description)
中国五级行政区划代码查询

数据来自: https://github.com/adyliu/china_area

## 使用方式 (Usage)  
提供两种检索方法:
### function 1
进入项目根目录下打开终端
```
# 编译程序
gcc -o search test.c

# 运行程序
./test
```

### function 2 (sql)
进入项目根目录下打开终端
```
# 在 Ubuntu/Debian 上安装 sqlite3 开发库
sudo apt-get install libsqlite3-dev

# 编译程序
gcc -o import import.c -lsqlite3
gcc -o query query.c -lsqlite3

# 确保 area_code_2024.csv 文件存在

# 导入数据
./import

# 查询数据
./query
```