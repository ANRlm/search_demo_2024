#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NAME_LENGTH 100
#define MAX_CODE_LENGTH 20
#define MAX_LINE_LENGTH 1024
#define MAX_REGIONS 700000

struct Region {
    char code[MAX_CODE_LENGTH];
    char name[MAX_NAME_LENGTH];
    int level;
    char parent_code[MAX_CODE_LENGTH];
    int type;
};

int loadRegionsFromCSV(struct Region regions[], const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("无法打开文件 %s\n", filename);
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
    
    printf("开始加载数据...\n");

    while (fgets(line, MAX_LINE_LENGTH, file) && count < MAX_REGIONS) {
        line[strcspn(line, "\n")] = 0;
        
        char *temp_line = strdup(line);
        if (temp_line == NULL) continue;

        char *temp_code = strtok(temp_line, ",");
        if (temp_code == NULL) {
            free(temp_line);
            continue;
        }
        strncpy(regions[count].code, temp_code, MAX_CODE_LENGTH - 1);
        regions[count].code[MAX_CODE_LENGTH - 1] = '\0';

        char *temp_name = strtok(NULL, ",");
        if (temp_name == NULL) {
            free(temp_line);
            continue;
        }
        strncpy(regions[count].name, temp_name, MAX_NAME_LENGTH - 1);
        regions[count].name[MAX_NAME_LENGTH - 1] = '\0';

        char *temp_level = strtok(NULL, ",");
        if (temp_level == NULL) {
            free(temp_line);
            continue;
        }
        regions[count].level = atoi(temp_level);

        char *temp_parent = strtok(NULL, ",");
        if (temp_parent == NULL) {
            free(temp_line);
            continue;
        }
        strncpy(regions[count].parent_code, temp_parent, MAX_CODE_LENGTH - 1);
        regions[count].parent_code[MAX_CODE_LENGTH - 1] = '\0';

        char *temp_type = strtok(NULL, ",");
        if (temp_type == NULL) {
            free(temp_line);
            continue;
        }
        regions[count].type = atoi(temp_type);

        free(temp_line);
        count++;
        
        if (count % 1000 == 0) {
            printf("\r已加载 %d 条数据...", count);
            fflush(stdout);
        }
    }

    fclose(file);
    printf("\n数据加载完成，共 %d 条记录\n", count);
    return count;
}

void findByCode(struct Region regions[], int size, const char* code) {
    int found = 0;
    for (int i = 0; i < size; i++) {
        if (strcmp(regions[i].code, code) == 0) {
            printf("找到地区：\n");
            printf("名称: %s\n", regions[i].name);
            printf("代码: %s\n", regions[i].code);
            printf("级别: %d\n", regions[i].level);
            printf("上级代码: %s\n", regions[i].parent_code);
            found = 1;
            
            if (strcmp(regions[i].parent_code, "0") != 0) {
                for (int j = 0; j < size; j++) {
                    if (strcmp(regions[j].code, regions[i].parent_code) == 0) {
                        printf("上级地区: %s\n", regions[j].name);
                        break;
                    }
                }
            }
            break;
        }
    }
    if (!found) {
        printf("未找到对应代码的地区\n");
    }
}

void findByName(struct Region regions[], int size, const char* name) {
    int found = 0;
    printf("\n正在搜索: '%s'\n", name);
    
    for (int i = 0; i < size; i++) {
        if (strstr(regions[i].name, name) != NULL) {
            printf("\n----------------------------------------\n");
            printf("名称: %s\n", regions[i].name);
            printf("代码: %s\n", regions[i].code);
            printf("级别: %d\n", regions[i].level);
            
            char current_parent[MAX_CODE_LENGTH];
            strcpy(current_parent, regions[i].parent_code);
            
            while (strcmp(current_parent, "0") != 0) {
                for (int j = 0; j < size; j++) {
                    if (strcmp(regions[j].code, current_parent) == 0) {
                        printf("上级: %s\n", regions[j].name);
                        strcpy(current_parent, regions[j].parent_code);
                        break;
                    }
                }
            }
            
            found++;
            
            if (found >= 20) {
                printf("\n结果过多，仅显示前20条...\n");
                break;
            }
        }
    }
    
    if (!found) {
        printf("未找到包含 '%s' 的地区\n", name);
    } else {
        printf("\n共找到 %d 个匹配项\n", found);
    }
}

int main() {
    struct Region *regions = malloc(MAX_REGIONS * sizeof(struct Region));
    if (regions == NULL) {
        printf("内存分配失败\n");
        return 1;
    }

    int size = loadRegionsFromCSV(regions, "area_code_2024.csv");
    
    if (size == 0) {
        printf("数据加载失败\n");
        free(regions);
        return 1;
    }
    
    printf("成功加载 %d 条区划数据\n", size);
    
    int choice;
    char search_term[MAX_NAME_LENGTH];
    
    while (1) {
        printf("\n请选择查询方式：\n");
        printf("1. 按代码查询地区\n");
        printf("2. 按地区名称查询代码\n");
        printf("3. 退出\n");
        printf("请输入选择 (1-3): ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("输入无效，请重试\n");
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                printf("请输入区划代码：");
                if (fgets(search_term, MAX_NAME_LENGTH, stdin) != NULL) {
                    search_term[strcspn(search_term, "\n")] = 0;
                    findByCode(regions, size, search_term);
                }
                break;
                
            case 2:
                printf("请输入地区名称：");
                if (fgets(search_term, MAX_NAME_LENGTH, stdin) != NULL) {
                    search_term[strcspn(search_term, "\n")] = 0;
                    findByName(regions, size, search_term);
                }
                break;
                
            case 3:
                printf("程序已退出\n");
                free(regions);
                return 0;
                
            default:
                printf("无效的选择，请重试\n");
        }
    }
    
    free(regions);
    return 0;
}