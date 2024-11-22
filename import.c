#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    
    // 打开/创建数据库
    int rc = sqlite3_open("areas.db", &db);
    if (rc != SQLITE_OK) {
        printf("无法打开数据库: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // 创建表
    const char *sql_create = 
        "DROP TABLE IF EXISTS areas;"
        "CREATE TABLE areas ("
        "code TEXT PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "level INTEGER,"
        "parent_code TEXT,"
        "type INTEGER,"
        "FOREIGN KEY(parent_code) REFERENCES areas(code)"
        ");"
        "CREATE INDEX idx_parent ON areas(parent_code);"
        "CREATE INDEX idx_name ON areas(name);";
    
    rc = sqlite3_exec(db, sql_create, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    // 准备插入语句
    sqlite3_stmt *stmt;
    const char *sql_insert = "INSERT INTO areas (code, name, level, parent_code, type) VALUES (?, ?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0);
    
    // 开始事务
    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
    
    // 读取CSV文件
    FILE *fp = fopen("area_code_2024.csv", "r");
    if (!fp) {
        printf("无法打开CSV文件\n");
        return 1;
    }
    
    char line[MAX_LINE_LENGTH];
    int count = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        line[strcspn(line, "\n")] = 0;
        
        char *code = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *level = strtok(NULL, ",");
        char *parent = strtok(NULL, ",");
        char *type = strtok(NULL, ",");
        
        if (!code || !name || !level || !parent || !type) {
            printf("警告：第 %d 行数据格式不正确，已跳过\n", count + 1);
            continue;
        }
        
        sqlite3_bind_text(stmt, 1, code, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, atoi(level));
        sqlite3_bind_text(stmt, 4, parent, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, atoi(type));
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            printf("插入失败: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_reset(stmt);
        
        count++;
        if (count % 1000 == 0) {
            printf("\r已导入 %d 条记录...", count);
            fflush(stdout);
        }
    }
    
    // 提交事务
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    
    printf("\n总共导入 %d 条记录\n", count);
    
    fclose(fp);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return 0;
}