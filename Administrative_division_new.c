/**
 * @file test.c
 * @brief 中国行政区划数据管理与查询系统
 * 
 * @details
 * 本程序实现了一个基于树结构的中国行政区划数据管理系统，主要功能包括：
 * 1. 从CSV文件加载行政区划数据
 * 2. 构建多级树状结构存储区划关系
 * 3. 支持按区划代码精确查询
 * 4. 支持按地区名称模糊查询
 * 5. 显示完整的行政区划层级关系
 * 6. 支持额外的统计数据（如房价、就业率等）
 * 
 * CSV文件格式要求：
 * - 文件名：area_code_2024_new.csv
 * - 必需字段：code,name,level,parent_code,type
 * - 可选字段：avg_house_price,employment_rate
 * 
 * 数据规模支持：
 * - 最大支持70万条区划数据
 * - 地区名称最大长度：100字符
 * - 区划代码最大长度：20字符
 * 
 * 行政区划级别说明：
 * - 0级：国家级
 * - 1级：省级（省、直辖市、自治区、特别行政区）
 * - 2级：地级（地级市、地区、自治州、盟）
 * - 3级：县级（市辖区、县级市、县、自治县、旗）
 * - 4级：乡级（街道、镇、乡、苏木）
 * - 5级：村级（居委会、村委会、嘎查）
 * 
 * @author [ANRlm]
 * @date 2024-12-08
 * 
 * @note
 * 编译环境要求：
 * - 支持C99标准
 * - 需要标准库：stdio.h, string.h, stdlib.h
 * 
 * @warning
 * - 请确保输入文件编码为UTF-8
 * - 内存占用与数据量成正比，请确保系统有足够内存
 * - 首次加载大量数据时可能需要较长时间
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 程序使用的常量定义
 */
#define MAX_NAME_LENGTH 100   // 地区名称最大长度，限制字符串大小
#define MAX_CODE_LENGTH 20    // 地区代码最大长度，如"110000000000"
#define MAX_LINE_LENGTH 1024  // CSV文件单行最大长度，用于读取缓冲区
#define MAX_REGIONS 700000    // 地区数据最大数量，根据实际数据量设置
#define HASH_SIZE 1000003     // 哈希表大小，选择大于数据量的质数以减少冲突

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
    char parent_code[MAX_CODE_LENGTH]; // 父级地区代码，"0"示无父级
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
 * @brief 向父节点添加子节点
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
 * @brief 比较两个节点的代码，用于qsort排序
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
 * @brief 构建地区树
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

    printf("开始创建节点...\n");
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
        
        if (i % 1000 == 0) {
            printf("\r已创建 %d/%d 个节点... (%.1f%%)", i, size, (float)i/size*100);
            fflush(stdout);
        }
    }
    printf("\n节点创建完成\n");

    // 对映射数组进行排序，以支持二分查找
    printf("正在对节点进行排序...\n");
    qsort(codeToNode, size, sizeof(*codeToNode), compareCodeToNode);
    printf("节点排序完成\n");

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

    printf("开始建立父子关系...\n");
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
        
        if (i % 1000 == 0) {
            printf("\r已处理 %d/%d 个节点的父子关系... (%.1f%%)", 
                   i, size, (float)i/size*100);
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
 * @brief 打印树结构（用于调试）
 * 
 * @param node 当前节点
 * @param level 当前节点的层级
 */
void printTree(struct TreeNode* node, int level) {
    if (node == NULL) return;

    for (int i = 0; i < level; i++) printf("  ");
    printf("%s - %s\n", node->data.code, node->data.name);

    for (int i = 0; i < node->child_count; i++) {
        printTree(node->children[i], level + 1);
    }
}

/**
 * @brief 按代码查找节点
 * @note 采用深度优先搜索算法遍历树结构
 * 时间复杂度：O(n)，其中n为节点总数
 * 空间复杂度：O(h)，其中h为树的高度
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
 * @brief 通过地区代码查找地区信息并打印，包括所有上级地区
 * 
 * @param root 地区树的根节点
 * @param code 要查找的地区代码
 */
void findByCode(struct TreeNode* node, const char* code) {
    struct TreeNode* found = findNodeByCode(node, code);
    if (found) {
        printf("代码: %s\n", found->data.code);
        printf("名称: %s\n", found->data.name);
        if (found->data.level >= 0 && found->data.level <= 5) {
            printf("级别: %s\n", LEVEL_NAMES[found->data.level]);
        } else {
            printf("级别: 未知(%d)\n", found->data.level);
        }
        printf("类型: %d\n", found->data.type);
        if (found->data.avg_house_price != NULL) {
            printf("平均房价: %.2lf\n", *found->data.avg_house_price);
        }
        if (found->data.employment_rate != NULL) {
            printf("就业率: %s\n", found->data.employment_rate);
        }

        // 使用父节点指针打印所有上级地区
        struct TreeNode* current = found;
        while (current->parent != NULL) {
            current = current->parent;
            if (current->data.level >= 0 && current->data.level <= 5) {
                printf("%s: %s\n", LEVEL_NAMES[current->data.level], current->data.name);
            } else {
                printf("未知级别(%d): %s\n", current->data.level, current->data.name);
            }
        }
    } else {
        printf("未找到代码为 %s 的地区\n", code);
    }
}

/**
 * @brief 从CSV文件中加载地区数据
 * @note 处理步骤
 * 1. 打开并检查CSV文件
 * 2. 读取第一行，判断是否为标题行
 * 3. 逐行解析数据，包括可选的房价和就业率字段
 * 4. 动态分配内存存储可选字段
 * 5. 定期显示加载进度
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
    printf("开始加载数据...\n");

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
                perror("内存分配失败");
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
    printf("\n数据加载完成，共 %d 条记录\n", count);
    return count;
}

/**
 * @brief 递归查找地区名称
 * @note 采用深度优先搜索算法，支持模糊匹配
 * 限制最多显示5条匹配结果
 * 对每个匹配项显示完整的行政区划层级
 * 
 * @param root 当前遍历的树节点
 * @param name 要查找的地区名称（支持部分匹配）
 * @param found 已找到的匹配项数量
 */
void findByNameRecursive(struct TreeNode* root, const char* name, int* found) {
    if (root == NULL || *found >= 5) return;
    
    // 检查当前节点
    if (strstr(root->data.name, name) != NULL) {
        printf("\n-------------------\n");
        printf("名称: %s\n", root->data.name);
        printf("代码: %s\n", root->data.code);
        if (root->data.level >= 0 && root->data.level <= 5) {
            printf("级别: %s\n", LEVEL_NAMES[root->data.level]);
        } else {
            printf("级别: 未知(%d)\n", root->data.level);
        }
        if (root->data.avg_house_price != NULL) {
            printf("平均房价: %.2lf\n", *root->data.avg_house_price);
        }
        if (root->data.employment_rate != NULL) {
            printf("就业率: %s\n", root->data.employment_rate);
        }
        
        // 打印所有上级地区
        struct TreeNode* current = findNodeByCode(root, root->data.code);
        while (current && strcmp(current->data.parent_code, "0") != 0) {
            struct TreeNode* parent = findNodeByCode(root, current->data.parent_code);
            if (parent) {
                if (parent->data.level >= 0 && parent->data.level <= 5) {
                    printf("%s: %s\n", LEVEL_NAMES[parent->data.level], parent->data.name);
                } else {
                    printf("未知级别(%d): %s\n", parent->data.level, parent->data.name);
                }
                current = parent;
            } else {
                break;
            }
        }
        
        (*found)++;
        if (*found >= 5) {
            printf("\n结果过多，仅显示前5条...\n");
            return;
        }
    }
    
    // 递归搜索子节点
    for (int i = 0; i < root->child_count; i++) {
        findByNameRecursive(root->children[i], name, found);
    }
}

/**
 * @brief 通过地区名称查找地区信息并打印
 * 
 * @param root 地区树的根节点
 * @param name 要查找的地区名称
 * @note 最多显示20条匹配结果
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
 * @brief 释放树结构占用的内存
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
 * @brief 程序主入口
 * @note 主要流程：
 * 1. 分配内存存储地区数据
 * 2. 从CSV文件加载数据
 * 3. 构建树形结构
 * 4. 进入交互式查询循环
 * 5. 程序退出时释放所有资源
 * 
 * @return 程序执行状态码：0-成功，1-失败
 */
int main() {
    struct Region *regions = malloc(MAX_REGIONS * sizeof(struct Region));
    if (regions == NULL) {
        perror("内存分配失败");
        return 1;
    }

    int size = loadRegionsFromCSV(regions, "area_code_2024_new.csv");

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
                    findByName(root, search_term);
                }
                break;

            case 3:
                printf("程序已退出\n");
                freeTree(root);  // 释放树结构
                for (int i = 0; i < size; i++) {
                    free(regions[i].avg_house_price);
                    free(regions[i].employment_rate);
                }
                free(regions);
                return 0;

            default:
                printf("无效的选择，请重试\n");
        }
    }

    for (int i = 0; i < size; i++) {
        free(regions[i].avg_house_price);
        free(regions[i].employment_rate);
    }
    free(regions);
    return 0;
}