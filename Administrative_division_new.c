/**
 * @file Administrative_division_new.c
 * @brief 中国行政区划数据管理与查询系统
 * 
 * 功能：
 * - 从CSV文件加载区划数据
 * - 构建树状结构存储区划关系
 * - 支持代码精确查询和名称模糊查询
 * - 显示完整的行政区划层级关系
 * 
 * CSV格式：code,name,level,parent_code,type[,avg_house_price,employment_rate]
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 程序使用的常量定义
 */
#define MAX_NAME_LENGTH 100   // 地区名称最大长度
#define MAX_CODE_LENGTH 20    // 地区代码最大长度
#define MAX_LINE_LENGTH 1024  // CSV文件单行最大长度
#define MAX_REGIONS 700000    // 地区数据最大数量

/**
 * @brief 地区级别的名称数组
 * @note 索引对应级别编号：0-国家级，1-省级，2-地级，3-县级，4-乡级，5-村级
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
 * @brief 结构体，表示一个地区的信息
 * @note 包含地区的基本信息和扩展数据
 */
struct Region {
    char code[MAX_CODE_LENGTH];     // 地区代码，如"110000000000"
    char name[MAX_NAME_LENGTH];     // 地区名称，如"北京市"
    int level;                      // 地区级别，0-5分别对应不同行政级别
    char parent_code[MAX_CODE_LENGTH]; // 父级地区代码，"0"表示无父级
    int type;                       // 地区类型，用于区分特殊行政区等
    double *avg_house_price;        // 平均房价，可选字段，NULL表示无数据
    char *employment_rate;          // 就业率，可选字段，NULL表示无数据
};

typedef struct Region Region;

/**
 * @brief 树节点结构体，用于构建地区树
 * @note 采用子节点数组实现的树结构，支持动态扩容
 */
struct TreeNode {
    struct Region data;             // 节点存储的地区信息
    struct TreeNode** children;     // 子节点指针数组，动态分配
    struct TreeNode* parent;        // 父节点指针，用于快速向上查找
    int child_count;               // 当前子节点数量
    int child_capacity;            // 子节点数组容量，支持动态扩容
};

/**
 * @brief 创建一个新的树节点
 * 
 * @param data 地区信息
 * @return 新建的树节点指针，失败返回NULL
 */
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
    node->parent = NULL;  // 初始化父节点指针为NULL
    return node;
}

/**
 * @brief 向父节点添加子节点，支持动态扩容
 * 
 * @param parent 父节点指针
 * @param child 子节点指针
 */
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

/**
 * @brief 比较函数，用于节点排序
 * 
 * @param a 第一个节点的指针
 * @param b 第二个节点的指针
 * @return int 比较结果：负数表示a<b，0表示a=b，正数表示a>b
 */
static int compareCodeToNode(const void* a, const void* b) {
    const struct {
        char code[MAX_CODE_LENGTH];
        struct TreeNode* node;
    } *nodeA = a, *nodeB = b;
    
    return strcmp(nodeA->code, nodeB->code);
}

/**
 * @brief 构建行政区划树
 * @note 采用二分查找优化父子关系建立
 * 
 * @param regions 地区信息数组
 * @param size 地区信息数组大小
 * @return 根节点指针，失败返回NULL
 */
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

/**
 * @brief 按代码查找节点 (DFS算法)
 * @complexity 时间O(n), 空间O(h), h为树高
 * 
 * @param root 搜索的起始节点
 * @param code 要查找的地区代码
 * @return 找到的节点指针，未找到返回NULL
 */
struct TreeNode* findNodeByCode(struct TreeNode* root, const char* code) {
    if (root == NULL) return NULL;
    if (strcmp(root->data.code, code) == 0) return root;

    for (int i = 0; i < root->child_count; i++) {
        struct TreeNode* found = findNodeByCode(root->children[i], code);
        if (found != NULL) return found;
    }
    return NULL;
}

/**
 * @brief 按代码查询并显示完整行政层级
 * 
 * @param root 地区树的根节点
 * @param code 要查找的地区代码
 */
void findByCode(struct TreeNode* node, const char* code) {
    struct TreeNode* found = findNodeByCode(node, code);
    if (found) {
        printf("名称: %s\n", found->data.name);
        printf("代码: %s\n", found->data.code);
        if (found->data.level >= 0 && found->data.level <= 5) {
            printf("级别: %s\n", LEVEL_NAMES[found->data.level]);
        } else {
            printf("级别: 未知(%d)\n", found->data.level);
        }
        printf("类型: %d\n", found->data.type);
        
        if (found->data.avg_house_price != NULL && *found->data.avg_house_price > 0) {
            printf("平均房价: %.2lf\n", *found->data.avg_house_price);
        } else {
            printf("平均房价: 暂无数据\n");
        }
        
        if (found->data.employment_rate != NULL && 
            strcmp(found->data.employment_rate, "N/A") != 0) {
            printf("就业率: %s\n", found->data.employment_rate);
        } else {
            printf("就业率: 暂无数据\n");
        }
        printf("行政区划层级关系：\n└─ %s", found->data.name);  // 移除换行符
        
        struct TreeNode* current = found;
        while (current->parent != NULL && current->parent->data.level != 0) {
            current = current->parent;
            if (current->data.level >= 0 && current->data.level <= 5) {
                printf("\n   └─ 隶属于%s：%s", LEVEL_NAMES[current->data.level], current->data.name);
            } else {
                printf("\n   └─ 隶属于未知级别(%d)：%s", current->data.level, current->data.name);
            }
        }
        printf("\n");  // 最后添加一个换行符
    } else {
        printf("未找到代码为 %s 的地区\n", code);
    }
}

/**
 * @brief 从CSV加载数据
 * @note 支持可选的房价和就业率字段
 * 
 * @param regions 用于存储解析结果的地区数组
 * @param filename CSV文件名
 * @return 成功加载的地区数量，失败返回0
 */
int loadRegionsFromCSV(struct Region regions[], const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("无法打开文件");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
    printf("开始加载数据...");

    // 读取第一行，检查是否为标题行
    if (fgets(line, MAX_LINE_LENGTH, file)) {
        // 尝试解析第一行
        double avg_price = 0.0;
        char employment_rate_str[20] = "N/A";
        int parsed_items = sscanf(line, "%19[^,],%99[^,],%d,%19[^,],%d,%lf,%19[^,]",
                                regions[count].code,
                                regions[count].name,
                                &regions[count].level,
                                regions[count].parent_code,
                                &regions[count].type,
                                &avg_price,
                                employment_rate_str);

        // 如果第一行可以被解析为数据，则处理它
        if (parsed_items >= 5) {
            regions[count].avg_house_price = malloc(sizeof(double));
            if (regions[count].avg_house_price == NULL) {
                perror("内存分配失败");
                fclose(file);
                return 0;
            }
            *regions[count].avg_house_price = (parsed_items >= 6) ? avg_price : 0.0;

            regions[count].employment_rate = strdup((parsed_items >= 7) ? employment_rate_str : "N/A");
            if (regions[count].employment_rate == NULL) {
                perror("内存分配失败");
                free(regions[count].avg_house_price);
                fclose(file);
                return 0;
            }
            count++;
        }
        // 如果第一行不能被解析为数据，则认为它是标题行，继续处理后面的行
    }

    while (fgets(line, MAX_LINE_LENGTH, file) && count < MAX_REGIONS) {
        // 去除尾随换行符
        line[strcspn(line, "\n")] = 0;

        double avg_price = 0.0;
        char employment_rate_str[20] = "N/A";
        int parsed_items;

        parsed_items = sscanf(line, "%19[^,],%99[^,],%d,%19[^,],%d,%lf,%19[^,]",
                            regions[count].code,
                            regions[count].name,
                            &regions[count].level,
                            regions[count].parent_code,
                            &regions[count].type,
                            &avg_price,
                            employment_rate_str);

        if (parsed_items >= 5) {
            regions[count].avg_house_price = malloc(sizeof(double));
            if (regions[count].avg_house_price == NULL) {
                perror("内存分配失败");
                continue;
            }
            *regions[count].avg_house_price = (parsed_items >= 6) ? avg_price : 0.0;

            regions[count].employment_rate = strdup((parsed_items >= 7) ? employment_rate_str : "N/A");
            if (regions[count].employment_rate == NULL) {
                printf("错误：内存分配失败\n");
                free(regions[count].avg_house_price);
                continue;
            }
            count++;
        } else {
            fprintf(stderr, "Error parsing line: %s\n", line);
        }

        if (count % 1000 == 0) {
            printf("\r已加载 %d 条数据...", count);
            fflush(stdout);
        }
    }

    fclose(file);
    printf("\r数据加载完成，共 %d 条记录\n", count);
    return count;
}

/**
 * @brief 递归查找地区名称
 */
void findByNameRecursive(struct TreeNode* root, const char* name, int* found) {
    if (root == NULL || *found >= 5) return;
    
    if (strstr(root->data.name, name) != NULL) {
        if (*found > 0) {
            printf("----------------------------------------\n");
        }
        printf("名称: %s\n", root->data.name);
        printf("代码: %s\n", root->data.code);
        if (root->data.level >= 0 && root->data.level <= 5) {
            printf("级别: %s\n", LEVEL_NAMES[root->data.level]);
        } else {
            printf("级别: 未知(%d)\n", root->data.level);
        }
        
        if (root->data.avg_house_price != NULL && *root->data.avg_house_price > 0) {
            printf("平均房价: %.2lf\n", *root->data.avg_house_price);
        } else {
            printf("平均房价: 暂无数据\n");
        }
        
        if (root->data.employment_rate != NULL && 
            strcmp(root->data.employment_rate, "N/A") != 0) {
            printf("就业率: %s\n", root->data.employment_rate);
        } else {
            printf("就业率: 暂无数据\n");
        }
        printf("行政区划层级关系：\n└─ %s", root->data.name);
        
        struct TreeNode* current = root;
        while (current->parent != NULL && current->parent->data.level != 0) {
            current = current->parent;
            if (current->data.level >= 0 && current->data.level <= 5) {
                printf("\n   └─ 隶属于%s：%s", LEVEL_NAMES[current->data.level], current->data.name);
            } else {
                printf("\n   └─ 隶属于未知级别(%d)：%s", current->data.level, current->data.name);
            }
        }
        printf("\n");
        
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

/**
 * @brief 按名称查询接口
 * 
 * @param root 地区树的根节点
 * @param name 要查找的地区名称
 * @note 最多显示5条匹配结果
 */
void findByName(struct TreeNode* root, const char* name) {
    if (root == NULL) return;
    
    // 分配计数器内存，用于跟踪到的匹配项数量
    int* count = (int*)calloc(1, sizeof(int));
    if (count == NULL) {
        perror("内存分配失败");
        return;
    }
    
    // 开始递归搜索
    findByNameRecursive(root, name, count);
    
    // 打印搜索结果统计
    if (*count == 0) {
        printf("未找到包含 '%s' 的地区\n", name);
    } else {
        printf("\n共找到 %d 个匹配项\n", *count);
    }
    
    // 释放计数器内存
    free(count);
}

/**
 * @brief 释放树结构内存
 * 
 * @param root 要释放的树的根节点
 * @note 会递归释放所有子节点
 */
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

/**
 * @brief 主程序入口
 * @note 提供交互式查询界面
 * 
 * @return 程序执行状态码：0-成功，1-失败
 */
int main() {
    struct Region *regions = malloc(MAX_REGIONS * sizeof(struct Region));
    if (regions == NULL) {
        perror("内存分配失败");
        return 1;
    }

    printf("\n=== 中国行政区划数据管理与查询系统 ===\n");
    
    int size = loadRegionsFromCSV(regions, "area_code_2024_new.csv");

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
                printf("\n=== 按代码查询 ===\n");
                printf("请输入12位区划代码：");
                if (fgets(search_term, MAX_NAME_LENGTH, stdin) != NULL) {
                    search_term[strcspn(search_term, "\n")] = 0;
                    if (strlen(search_term) == 0) {
                        printf("\n代码不能为空\n");
                        continue;
                    }
                    printf("\n┌────────────── 查询结果 ──────────────┐\n\n");
                    findByCode(root, search_term);
                    printf("\n└─────────────────────────────────────┘\n");
                }
                break;

            case 2:
                printf("\n=== 按名称查询 ===\n");
                printf("请输入地区名称：");
                if (fgets(search_term, MAX_NAME_LENGTH, stdin) != NULL) {
                    search_term[strcspn(search_term, "\n")] = 0;
                    if (strlen(search_term) == 0) {
                        printf("\n名称不能为空\n");
                        continue;
                    }
                    printf("\n┌────────────── 查询结果 ──────────────┐\n\n");
                    findByName(root, search_term);
                    printf("\n└─────────────────────────────────────┘\n");
                }
                break;

            case 3:
                printf("\n=== 正在退出系统 ===\n");
                printf("释放树结构...\n");
                freeTree(root);  // 释放树结构
                for (int i = 0; i < size; i++) {
                    free(regions[i].avg_house_price);
                    free(regions[i].employment_rate);
                }
                free(regions);
                return 0;

            default:
                printf("\n无效的选择，请输入 1-3\n");
        }
    }

    return 0;
}