#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

void searchByName(sqlite3 *db, const char *name) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "WITH RECURSIVE ancestors AS ("
        "  SELECT code, name, level, parent_code, 0 as depth"
        "  FROM areas WHERE name LIKE ?"
        "  UNION ALL"
        "  SELECT a.code, a.name, a.level, a.parent_code, ancestors.depth + 1"
        "  FROM areas a"
        "  JOIN ancestors ON a.code = ancestors.parent_code"
        ")"
        "SELECT code, name, level, parent_code, depth FROM ancestors"
        " ORDER BY depth DESC, code LIMIT 100";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("查询准备失败: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    char search_term[256];
    snprintf(search_term, sizeof(search_term), "%%%s%%", name);
    sqlite3_bind_text(stmt, 1, search_term, -1, SQLITE_STATIC);
    
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (found == 0) {
            printf("\n找到以下匹配项：\n");
        }
        found++;
        
        const char *code = (const char *)sqlite3_column_text(stmt, 0);
        const char *area_name = (const char *)sqlite3_column_text(stmt, 1);
        int level = sqlite3_column_int(stmt, 2);
        int depth = sqlite3_column_int(stmt, 4);
        
        // 缩进显示层级关系
        for (int i = 0; i < depth; i++) {
            printf("  ");
        }
        printf("%s - %s (级别: %d)\n", code, area_name, level);
    }
    
    if (!found) {
        printf("未找到包含 '%s' 的地区\n", name);
    }
    
    sqlite3_finalize(stmt);
}

int main() {
    sqlite3 *db;
    int rc = sqlite3_open("areas.db", &db);
    if (rc != SQLITE_OK) {
        printf("无法打开数据库: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    char input[100];
    while (1) {
        printf("\n请输入要查询的地区名称（输入 'q' 退出）：");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 去除换行符
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "q") == 0) {
            break;
        }
        
        searchByName(db, input);
    }
    
    sqlite3_close(db);
    return 0;
}