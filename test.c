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

// 定义树节点结构
struct TreeNode {
    struct Region data;
    struct TreeNode** children;
    int child_count;
    int child_capacity;
};

// 创建树节点的函数
struct TreeNode* createNode(struct Region data) {
    struct TreeNode* node = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    node->data = data;
    node->child_capacity = 10;
    node->children = (struct TreeNode**)malloc(node->child_capacity * sizeof(struct TreeNode*));
    node->child_count = 0;
    return node;
}

// 添加子节点
void addChild(struct TreeNode* parent, struct TreeNode* child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = (struct TreeNode**)realloc(parent->children, 
            parent->child_capacity * sizeof(struct TreeNode*));
    }
    parent->children[parent->child_count++] = child;
}

// 构建树结构
struct TreeNode* buildTree(struct Region regions[], int size) {
    struct TreeNode* root = NULL;
    struct TreeNode** nodes = (struct TreeNode**)malloc(size * sizeof(struct TreeNode*));
    
    if (nodes == NULL) {
        printf("内存分配失败\n");
        return NULL;
    }
    
    printf("开始创建节点...\n");
    // 创建所有节点
    for (int i = 0; i < size; i++) {
        nodes[i] = createNode(regions[i]);
        if (nodes[i] == NULL) {
            printf("创建节点失败: %d\n", i);
            // 清理已创建的节点
            for (int j = 0; j < i; j++) {
                free(nodes[j]->children);
                free(nodes[j]);
            }
            free(nodes);
            return NULL;
        }
        if (i % 10000 == 0) {
            printf("\r已创建 %d 个节点...", i);
            fflush(stdout);
        }
    }
    printf("\n节点创建完成\n");
    
    printf("开始建立父子关系...\n");
    // 建立父子关系
    for (int i = 0; i < size; i++) {
        if (strcmp(regions[i].parent_code, "0") == 0) {
            root = nodes[i];
            printf("找到根节点: %s - %s\n", root->data.code, root->data.name);
        } else {
            for (int j = 0; j < size; j++) {
                if (strcmp(regions[i].parent_code, regions[j].code) == 0) {
                    addChild(nodes[j], nodes[i]);
                    break;
                }
            }
        }
        if (i % 10000 == 0) {
            printf("\r已处理 %d 个节点的父子关系...", i);
            fflush(stdout);
        }
    }
    printf("\n父子关系建立完成\n");
    
    if (root == NULL) {
        printf("错误：未找到根节点\n");
        // 清理所有节点
        for (int i = 0; i < size; i++) {
            free(nodes[i]->children);
            free(nodes[i]);
        }
        free(nodes);
        return NULL;
    }
    
    free(nodes);
    return root;
}

// 打印树结构（用于调试）
void printTree(struct TreeNode* node, int level) {
    if (node == NULL) return;
    
    for (int i = 0; i < level; i++) printf("  ");
    printf("%s - %s\n", node->data.code, node->data.name);
    
    for (int i = 0; i < node->child_count; i++) {
        printTree(node->children[i], level + 1);
    }
}

// 按代码查找节点
struct TreeNode* findNodeByCode(struct TreeNode* root, const char* code) {
    if (root == NULL) return NULL;
    if (strcmp(root->data.code, code) == 0) return root;
    
    for (int i = 0; i < root->child_count; i++) {
        struct TreeNode* found = findNodeByCode(root->children[i], code);
        if (found != NULL) return found;
    }
    return NULL;
}

// 修改后的findByCode函数
void findByCode(struct TreeNode* root, const char* code) {
    struct TreeNode* node = findNodeByCode(root, code);
    if (node != NULL) {
        printf("找到地区：\n");
        printf("名称: %s\n", node->data.name);
        printf("代码: %s\n", node->data.code);
        printf("级别: %d\n", node->data.level);
        printf("类型: %d\n", node->data.type);
        
        // 打印所有上级地区
        struct TreeNode* current = root;
        char* parent_code = node->data.parent_code;
        while (strcmp(parent_code, "0") != 0) {
            struct TreeNode* parent = findNodeByCode(root, parent_code);
            if (parent != NULL) {
                printf("上级: %s\n", parent->data.name);
                parent_code = parent->data.parent_code;
            } else {
                break;
            }
        }
    } else {
        printf("未找到对应代码的地区\n");
    }
}

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
    
    struct TreeNode* root = buildTree(regions, size);
    if (root == NULL) {
        printf("树结构构建失败\n");
        free(regions);
        return 1;
    }
    printf("树结构构建完成\n");
    
    // 打印根节点信息和直接子节点数量
    printf("根节点信息：\n");
    printf("名称: %s\n", root->data.name);
    printf("代码: %s\n", root->data.code);
    printf("直接子节点数量: %d\n", root->child_count);
    
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
                    findByCode(root, search_term);
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