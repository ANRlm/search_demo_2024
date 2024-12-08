/**
 * @file Administrative division.c
 * @brief 该程序用于构建和管理一个地区树结构，并提供按代码和名称搜索地区信息的功能。
 * csv文件格式：code,name,level,parent_code,type, 文件名：area_code_2024.csv
 */

/**
 * @brief:简要说明
 *
 * @param:描述函数的参数
 * @return:描述函数的返回值
 * @details:详细说明，可以用来补充更多背景信息
 */

#include <_inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

// 定义常量，限制字符串长度和区域数量
#define MAX_NAME_LENGTH 100  // 地区名称最大长度
#define MAX_CODE_LENGTH 20   // 地区代码最大长度
#define MAX_LINE_LENGTH 1024 // CSV文件单行最大长度
#define MAX_REGIONS 700000   // 地区数据最大数量

/**
 * @brief 结构体，表示一个地区的信息
 */
struct Region {
  char code[MAX_CODE_LENGTH];        // 地区代码
  char name[MAX_NAME_LENGTH];        // 地区名称
  int level;                         // 地区级别
  char parent_code[MAX_CODE_LENGTH]; // 父级地区代码
  int type;                          // 地区类型
};

/**
 * @brief 树节点结构体，用于构建地区树
 */
struct TreeNode {
  struct Region data;         // 地区信息
  struct TreeNode **children; // 子节点指针数组
  int child_count;            // 子节点数量
  int child_capacity;         // 子节点数组容量
};

/**
 * @brief 创建一个新的树节点
 *
 * @param data 地区信息
 * @return 新建的树节点指针，失败返回NULL
 */
struct TreeNode *createNode(struct Region data) {
  struct TreeNode *node = (struct TreeNode *)malloc(sizeof(struct TreeNode));

  if (node == NULL) {
    perror("内存分配失败");
    return NULL;
  }

  node->data = data;
  node->child_capacity = 10;
  node->children = (struct TreeNode **)malloc(node->child_capacity *
                                              sizeof(struct TreeNode *));

  if (node->children == NULL) {
    perror("内存分配失败");
    free(node);
    return NULL;
  }

  node->child_count = 0;
  return node;
}

/**
 * @brief 向父节点添加子节点
 *
 * @param parent 父节点指针
 * @param child 子节点指针
 */
void addChild(struct TreeNode *parent, struct TreeNode *child) {
  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity *= 2;
    parent->children = (struct TreeNode **)realloc(
        parent->children, parent->child_capacity * sizeof(struct TreeNode *));

    if (parent->children == NULL) {
      perror("内存重新分配失败");
      exit(1); // 发生错误时退出程序
    }
  }

  parent->children[parent->child_count++] = child;
}

/**
 * @brief 构建地区树
 *
 * @param regions 地区信息数组
 * @param size 地区信息数组大小
 * @return 根节点指针，失败返回NULL
 */
struct TreeNode *buildTree(struct Region regions[], int size) {
  struct TreeNode *root = NULL;
  struct TreeNode **nodes =
      (struct TreeNode **)malloc(size * sizeof(struct TreeNode *));

  if (nodes == NULL) {
    perror("内存分配失败");
    return NULL;
  }

  printf("开始创建节点...\n");
  // 创建所有节点
  for (int i = 0; i < size; i++) {
    nodes[i] = createNode(regions[i]);

    if (nodes[i] == NULL) {
      printf("创建节点失败: %d\n", i);
      // 清理已经创建的节点
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
    printf("错误: 未找到根节点\n");
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

// /**
//  * @brief 打印树结构 (用于调试)
//  *
//  * @param node 当前节点
//  * @param level 当前节点的层级
//  */
// void printTree(struct TreeNode* node, int level) {
//   if (node == NULL) return;

//   for (int i = 0;i < level; i++) printf(" ");
//   printf("%s - %s\n", node->data.code, node->data.name);

//   for (int i = 0; i < node->child_count; i++) {
//     printTree(node->children[i], level + 1);
//   }
// }

/**
 * @brief 按代码查找节点
 *
 * @param root 根节点
 * @param code 要查找的地区代码
 * @return 找到的节点指针，未找到返回NULL
 */
struct TreeNode *findNodeByCode(struct TreeNode *root, const char *code) {
  if (root == NULL)
    return NULL;
  if (strcmp(root->data.code, code) == 0)
    return root;

  for (int i = 0; i < root->child_count; i++) {
    struct TreeNode *found = findNodeByCode(root->children[i], code);
    if (found != NULL)
      return found;
  }
  return NULL;
}

/**
 * @brief 通过地区代码查找地区信息并打印
 *
 * @param root 地区数的根节点
 * @param code 要查找的地区代码
 */
void findByCode(struct TreeNode *root, const char *code) {
  struct TreeNode *node = findNodeByCode(root, code);
  if (node != NULL) {
    printf("找到地区: \n");
    printf("名称: %s\n", node->data.name);
    printf("代码: %s\n", node->data.code);
    printf("级别: %d\n", node->data.level);
    printf("类型: %d\n", node->data.type);

    // 打印所有上级地区
    struct TreeNode *current = node;
    char *parent_code = current->data.parent_code;
    while (strcmp(parent_code, "0") != 0) {
      struct TreeNode *parent = findNodeByCode(root, parent_code);
      if (parent != NULL) {
        printf("上级: %s\n", parent->data.name);
        parent_code = parent->data.parent_code;
        current = parent;
      } else
        break;
    }
  } else
    printf("未找到对应代码的地区\n");
}

/**
 * @brief 从CSV文件中加载地区数据
 *
 * @param regions 地区信息数组
 * @param filename CSV文件名
 * @return 加载的地区数量
 */
int loadRegionsFromCSV(struct Region regions[], const char *filename) {
  FILE *file = fopen(filename, "r");

  if (file == NULL) {
    perror("无法打开文件");
    return 0;
  }

  char line[MAX_LINE_LENGTH];
  int count = 0;

  printf("开始加载数据...\n");

  while (fgets(line, MAX_LINE_LENGTH, file) && count < MAX_REGIONS) {
    // 去除换行符
    line[strcspn(line, "\n")] = 0;

    // 使用strdup复制字符串，避免修改原始字符串
    char *temp_line = strdup(line);
    if (temp_line == NULL) {
      perror("内存分配失败");
      continue;
    }

    // 使用strtok分割字符串
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
  printf("\n数据加载完成, 共 %d 条记录\n", count);
  return count;
}

/**
 * @brief 通过地区名称查找地区信息并且打印
 *
 * @param regions 地区信息数组
 * @param size 地区信息数组大小
 * @param name 要查找的地区名称
 */
void findByName(struct Region regions[], int size, const char *name) {
  int found = 0;
  printf("\n正在搜索: '%s'\n", name);

  for (int i = 0; i < size; i++) {
    if (strstr(regions[i].name, name) != NULL) {
      printf("\n--------------------\n");
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
      if (found >= 10) {
        printf("\n结果过多, 仅显示前10条...\n");
        break;
      }
    }
  }
  if (!found) {
    printf("未找到包含 '%s' 的地区\n", name);
  } else
    printf("\n共找到 %d 个匹配项\n", found);
}

int main() {
  struct Region *regions = malloc(MAX_REGIONS * sizeof(struct Region));
  if (regions == NULL) {
    perror("内存分配失败");
    return 1;
  }

  int size = loadRegionsFromCSV(regions, "area_code_2024.csv");

  if (size == 0) {
    printf("数据加载失败\n");
    free(regions);
    return 1;
  }

  printf("成功加载 %d 条区划数据\n", size);

  struct TreeNode *root = buildTree(regions, size);

  if (root == NULL) {
    printf("树结构构建失败\n");
    free(regions);
    return 1;
    ;
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
      while (getchar() != 1) {
        printf("输入无效, 请重试\n");
        continue;
      }
      while (getchar() != '\n')
        ;

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
    ;
  }
}
