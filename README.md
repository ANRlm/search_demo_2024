# Search Demo 2024

## 项目简介 (Project Description)
中国五级行政区划代码查询，纯 C 语言实现，无第三方依赖。

区划数据来自: https://github.com/adyliu/china_area

本项目仅使用 area_code_2024.csv 和 area_code_2024.sql 文件中的数据。

仅用于学习交流，请勿用于其他用途。

## 使用方式 (Usage)  
以下提供两种检索方法，请确保已安装 gcc 编译器。
### function 1
进入项目根目录下打开终端
```
# 编译程序
gcc -o test test.c

# 运行程序
./test
```

### function 2 (sql)
进入项目根目录下打开终端, 确保已安装 sqlite3 开发库
```
# 在 Ubuntu/Debian 上安装 sqlite3 开发库
sudo apt-get install libsqlite3-dev

# windows 安装 sqlite3 开发库
https://www.sqlite.org/download.html

# macos 安装 sqlite3 开发库
brew install sqlite3
```

```
# 编译程序
gcc -o import import.c -lsqlite3
gcc -o query query.c -lsqlite3

# 导入数据
./import

# 查询数据
./query
```