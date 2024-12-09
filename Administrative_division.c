/**
 * @file Administrative_division.c
 * @brief 中国行政区划数据管理与查询系统
 * @author ANRlm
 * @date 2024-12-09
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief 系统常量定义
 * @{
 */
#define MAX_NAME_LENGTH 100    ///< 地区名称最大字符数
#define MAX_CODE_LENGTH 20     ///< 地区代码最大字符数
#define MAX_LINE_LENGTH 1024   ///< CSV单行最大字符数
#define MAX_REGIONS 700000     ///< 系统支持的最大地区数量
/** @} */

/**
 * @brief 行政区划层级名称映射表
 * @details 数组索引对应行政级别：
 * - 0: 国家级
 * - 1: 省级（省、直辖市、自治区、特别行政区）
 * - 2: 地级（地级市、地区、自治州、盟）
 * - 3: 县级（市辖区、县级市、县、自治县、旗）
 * - 4: 乡级（街道、镇、乡、民族乡）
 * - 5: 村级（居委会、村委会）
 */
const char* LEVEL_NAMES[] = {
    "国家级(0)",
    "省级(1)",
    "地级(2)",
    "县级(3)",
    "乡级(4)",
    "村级(5)"
};

/**
 * @brief 行政区划实体结构
 * @details 含基本信息及扩展数据字段
 */
struct Region {
    char code[MAX_CODE_LENGTH];        ///< 区划代码
    char name[MAX_NAME_LENGTH];        ///< 区划名称
    int level;                         ///< 行政级别（0-5）
    char parent_code[MAX_CODE_LENGTH]; ///< 上级区划代码，"0"表示无上级
    int type;                          ///< 区划类型
    double *avg_house_price;           ///< 平均房价（可选）
    char *employment_rate;             ///< 就业率（可选）
};

/**
 * @brief 区划树节点结构
 * @details 采用动态数组存储子节点，支持自动扩容
 */
struct TreeNode {
    struct Region data;              ///< 节点数据
    struct TreeNode** children;      ///< 子节点指针数组
    struct TreeNode* parent;         ///< 父节点指针
    int child_count;                 ///< 当前子节点数量
    int child_capacity;              ///< 子节点数组容量
};

// === 函数声明部分 ===

// 树节点操作函数
struct TreeNode* createNode(struct Region data);
void addChild(struct TreeNode* parent, struct TreeNode* child);
static int compareCodeToNode(const void* a, const void* b);
struct TreeNode* buildTree(struct Region regions[], int size);
void freeTree(struct TreeNode* root);

// 数据查询函数
struct TreeNode* findNodeByCode(struct TreeNode* root, const char* code);
void findByNameRecursive(struct TreeNode* root, const char* name, int* found);
void findByCode(struct TreeNode* node, const char* code);
void findByName(struct TreeNode* root, const char* name);

// 数据验证函数
static int validateCode(const char* code);
static int validateName(const char* name);

// 数据显示函数
static void displayNodeInfo(struct TreeNode* node, int show_separator);

// 数据加载函数
int loadRegionsFromCSV(struct Region regions[], const char* filename);

// 用户界面函数
static int getInput(char* buffer, int max_len, const char* prompt);
int showMainMenu(struct TreeNode* root);

// === 函数实现部分 ===

// 1. 树节点操作函数组
struct TreeNode* createNode(struct Region data) {
    struct TreeNode* node = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    if (node == NULL) {
        perror("内存分配失败");
        return NULL;
    }
    node->data = data;
    node->child_capacity = 10;
    node->children = (struct TreeNode**)malloc(node->child_capacity * sizeof(struct TreeNode*));
    if (node->children == NULL) {
        perror("内存分配失败");
        free(node);
        return NULL;
    }
    node->child_count = 0;
    node->parent = NULL;  // 初始父节点指针为NULL
    return node;
}

void addChild(struct TreeNode* parent, struct TreeNode* child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = (struct TreeNode**)realloc(parent->children, 
            parent->child_capacity * sizeof(struct TreeNode*));
        if (parent->children == NULL) {
            perror("内存重新分配失败");
            exit(1);
        }
    }
    parent->children[parent->child_count++] = child;
    child->parent = parent;  // 设置子节点的父节点指针
}

static int compareCodeToNode(const void* a, const void* b) {
    const struct {
        char code[MAX_CODE_LENGTH];
        struct TreeNode* node;
    } *nodeA = a, *nodeB = b;
    
    return strcmp(nodeA->code, nodeB->code);
}

struct TreeNode* buildTree(struct Region regions[], int size) {
    // 为所有节点分配内存
    struct TreeNode** nodes = (struct TreeNode**)malloc(size * sizeof(struct TreeNode*));
    if (nodes == NULL) {
        perror("内存分配失败");
        return NULL;
    }

    // 创建用于快速查找的临时映射数组
    struct {
        char code[MAX_CODE_LENGTH];
        struct TreeNode* node;
    } *codeToNode = malloc(size * sizeof(*codeToNode));
    
    if (codeToNode == NULL) {
        free(nodes);
        perror("内存分配失败");
        return NULL;
    }

    printf("开始创建节点...");
    // 创建所有节点并建立索引
    for (int i = 0; i < size; i++) {
        nodes[i] = createNode(regions[i]);
        if (nodes[i] == NULL) {
            printf("创建节点失败: %d\n", i);
            for (int j = 0; j < i; j++) {
                free(nodes[j]->children);
                free(nodes[j]);
            }
            free(nodes);
            free(codeToNode);
            return NULL;
        }
        
        // 保存代码到节点的映射
        strncpy(codeToNode[i].code, regions[i].code, MAX_CODE_LENGTH - 1);
        codeToNode[i].code[MAX_CODE_LENGTH - 1] = '\0';
        codeToNode[i].node = nodes[i];
        
        if (i % 1000 == 0 || i == size - 1) {
            printf("\r已创建 %d/%d 个节点... (%.1f%%)", i + 1, size, (float)(i + 1)/size*100);
            fflush(stdout);
        }
    }
    printf("\n节点创建完成\n");

    printf("正在对节点进行排序...");
    qsort(codeToNode, size, sizeof(*codeToNode), compareCodeToNode);
    printf("完成\n");

    // 创建虚拟的全国根节点
    struct Region china = {
        .code = "000000000000",
        .name = "中华人民共和国",
        .level = 0,
        .parent_code = "0",
        .type = 0,
        .avg_house_price = NULL,
        .employment_rate = NULL
    };
    
    struct TreeNode* root = createNode(china);
    if (root == NULL) {
        printf("创建根节点失败\n");
        for (int i = 0; i < size; i++) {
            free(nodes[i]->children);
            free(nodes[i]);
        }
        free(nodes);
        free(codeToNode);
        return NULL;
    }

    printf("开始建立父子关系...");
    // 使用二分查找建立父子关系
    for (int i = 0; i < size; i++) {
        if (strcmp(regions[i].parent_code, "0") == 0) {
            // 省级节点直接添加到根节点下
            addChild(root, nodes[i]);
        } else {
            // 使用二分查找快速定位父节点
            struct TreeNode* parent = NULL;
            int left = 0, right = size - 1;
            
            while (left <= right) {
                int mid = (left + right) / 2;
                int cmp = strcmp(codeToNode[mid].code, regions[i].parent_code);
                
                if (cmp == 0) {
                    parent = codeToNode[mid].node;
                    break;
                } else if (cmp < 0) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            
            // 找到父节点后建立关系
            if (parent) {
                addChild(parent, nodes[i]);
            }
        }
        
        if (i % 1000 == 0 || i == size - 1) {
            printf("\r已处理 %d/%d 个节点的父子关系... (%.1f%%)", 
                   i + 1, size, (float)(i + 1)/size*100);
            fflush(stdout);
        }
    }
    printf("\n父子关系建立完成\n");

    // 清理临时数据结构
    free(codeToNode);
    free(nodes);
    return root;
}

void freeTree(struct TreeNode* root) {
    if (root == NULL) return;
    
    // 递归释放所有子节点
    for (int i = 0; i < root->child_count; i++) {
        freeTree(root->children[i]);
    }
    
    // 释放当前节点的资源
    free(root->children);
    free(root);
}

// 2. 数据查询函数组
struct TreeNode* findNodeByCode(struct TreeNode* root, const char* code) {
    if (root == NULL) return NULL;
    if (strcmp(root->data.code, code) == 0) return root;

    for (int i = 0; i < root->child_count; i++) {
        struct TreeNode* found = findNodeByCode(root->children[i], code);
        if (found != NULL) return found;
    }
    return NULL;
}

void findByNameRecursive(struct TreeNode* root, const char* name, int* found) {
    if (root == NULL || *found >= 5) return;
    
    if (strstr(root->data.name, name) != NULL) {
        displayNodeInfo(root, *found > 0);
        (*found)++;
        if (*found >= 5) {
            printf("\n结果过多，仅显示前5条...\n");
            return;
        }
    }
    
    for (int i = 0; i < root->child_count; i++) {
        findByNameRecursive(root->children[i], name, found);
    }
}

void findByCode(struct TreeNode* node, const char* code) {
    if (validateCode(code) != 0) {
        printf("错误：无效的区划代码格式\n");
        return;
    }
    
    struct TreeNode* found = findNodeByCode(node, code);
    if (found) {
        displayNodeInfo(found, 0);
    } else {
        printf("未找到代码为 %s 的地区\n", code);
    }
}

void findByName(struct TreeNode* root, const char* name) {
    if (!root || !name || validateName(name) != 0) {
        printf("错误：无效的查询名称\n");
        return;
    }
    
    int count = 0;
    findByNameRecursive(root, name, &count);
    
    if (count == 0) {
        printf("未找到包含 '%s' 的地区\n", name);
    } else {
        printf("\n共找到 %d 个匹配项\n", count);
    }
}

// 3. 数据验证函数组
static int validateCode(const char* code) {
    if (strlen(code) != 12) return -1;
    
    for (int i = 0; i < 12; i++) {
        if (!isdigit(code[i])) return -2;
    }
    return 0;
}

static int validateName(const char* name) {
    if (name == NULL || *name == '\0') return -1;
    
    int len = strlen(name);
    if (len >= MAX_NAME_LENGTH) return -2;
    
    // 检查是否只包含空白字符
    for (int i = 0; i < len; i++) {
        if (!isspace(name[i])) return 0;
    }
    return -3;
}

// 4. 数据显示函数组
static void displayNodeInfo(struct TreeNode* node, int show_separator) {
    if (node == NULL) return;
    
    if (show_separator) {
        printf("----------------------------------------\n");
    }
    
    // 基本信息显示
    printf("名称: %s\n", node->data.name);
    printf("代码: %s\n", node->data.code);
    printf("级别: %s\n", 
        (node->data.level >= 0 && node->data.level <= 5) ? 
        LEVEL_NAMES[node->data.level] : "未知级别");
    
    // 修改扩展数据显示部分
    if (node->data.avg_house_price && *node->data.avg_house_price > 0) {
        printf("平均房价: %.2f 元/平方米\n", *node->data.avg_house_price);
    } else {
        printf("平均房价: 暂无数据\n");
    }
    
    printf("就业率: %s\n", 
        (node->data.employment_rate && strcmp(node->data.employment_rate, "N/A") != 0) ? 
        node->data.employment_rate : "暂无数据");
    
    // 显示层级关系
    printf("行政区划层级关系：\n");
    struct TreeNode* current = node;
    int level = 0;
    
    while (current && current->parent) {
        for (int i = 0; i < level; i++) {
            printf("   ");
        }
        printf("└─ %s\n", current->data.name);
        current = current->parent;
        level++;
    }
}

// 5. 数据加载函数组
int loadRegionsFromCSV(struct Region regions[], const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("无法打开文件");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
    
    // 跳过标题行
    fgets(line, MAX_LINE_LENGTH, file);
    
    while (fgets(line, MAX_LINE_LENGTH, file) && count < MAX_REGIONS) {
        char* token = strtok(line, ",");
        if (!token) continue;
        
        // 基本字段解析
        strncpy(regions[count].code, token, MAX_CODE_LENGTH - 1);
        
        if (!(token = strtok(NULL, ","))) continue;
        strncpy(regions[count].name, token, MAX_NAME_LENGTH - 1);
        
        if (!(token = strtok(NULL, ","))) continue;
        regions[count].level = atoi(token);
        
        if (!(token = strtok(NULL, ","))) continue;
        strncpy(regions[count].parent_code, token, MAX_CODE_LENGTH - 1);
        
        if (!(token = strtok(NULL, ","))) continue;
        regions[count].type = atoi(token);
        
        // 可选字段处理
        regions[count].avg_house_price = malloc(sizeof(double));
        *regions[count].avg_house_price = (token = strtok(NULL, ",")) ? atof(token) : 0.0;
        
        regions[count].employment_rate = strdup((token = strtok(NULL, ",")) ? token : "N/A");
        
        count++;
    }
    
    fclose(file);
    return count;
}

// 6. 用户界面函数组
static int getInput(char* buffer, int max_len, const char* prompt) {
    printf("%s", prompt);
    if (!fgets(buffer, max_len, stdin)) {
        return -1;
    }
    
    // 去除换行符
    buffer[strcspn(buffer, "\n")] = 0;
    
    // 去除首尾空格
    char* start = buffer;
    while (*start && isspace(*start)) start++;
    
    if (*start == '\0') return -1;
    
    char* end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    if (start != buffer) {
        memmove(buffer, start, strlen(start) + 1);
    }
    
    return 0;
}

int showMainMenu(struct TreeNode* root) {
    int choice;
    char search_term[MAX_NAME_LENGTH];
    
    while (1) {
        printf("\n┌────────────────────────────────┐\n");
        printf("│     行政区划数据查询系统       │\n");
        printf("├────────────────────────────────┤\n");
        printf("│  1. 按代码查询地区信息         │\n");
        printf("│  2. 按名称查询地区信息         │\n");
        printf("│  3. 退出系统                   │\n");
        printf("└────────────────────────────────┘\n");
        printf("\n请输入选项编号 [1-3]: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("\n输入无效，请输入数字 1-3\n");
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            case 1:
                if (getInput(search_term, MAX_NAME_LENGTH, 
                    "\n=== 按代码查询 ===\n请输入12位区划代码：") != 0) {
                    printf("\n代码不能为空\n");
                    continue;
                }
                printf("\n┌────────────── 查询结果 ──────────────┐\n\n");
                findByCode(root, search_term);
                printf("\n└─────────────────────────────────────┘\n");
                break;

            case 2:
                if (getInput(search_term, MAX_NAME_LENGTH,
                    "\n=== 按名称查询 ===\n请输入地区名称：") != 0) {
                    printf("\n名称不能为空\n");
                    continue;
                }
                printf("\n┌────────────── 查询结果 ──────────────┐\n\n");
                findByName(root, search_term);
                printf("\n└─────────────────────────────────────┘\n");
                break;

            case 3:
                return 0;

            default:
                printf("\n无效的选择，请输入 1-3\n");
        }
    }
}

// 7. 主函数
int main() {
    struct Region *regions = malloc(MAX_REGIONS * sizeof(struct Region));
    if (regions == NULL) {
        perror("内存分配失败");
        return 1;
    }

    printf("\n=== 中国行政区划数据管理与查询系统 ===\n");
    
    int size = loadRegionsFromCSV(regions, "area_data.csv");

    if (size == 0) {
        printf("错误：数据加载失败\n");
        free(regions);
        return 1;
    }

    printf("成功加载 %d 条区划数据\n", size);

    struct TreeNode* root = buildTree(regions, size);
    if (root == NULL) {
        printf("错误：树结构构建失败\n");
        free(regions);
        return 1;
    }
    printf("树结构构建完成\n");

    int result = showMainMenu(root);
    printf("\n系统退出\n");
    
    // 释放资源
    freeTree(root);
    free(regions);
    
    return result;
}