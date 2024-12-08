/**
 * @file Administrative_division_new.c
 * @brief 行政区划数据管理系统
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LEN_NAME 100
#define LEN_CODE 20
#define LEN_LINE 1024
#define MAX_REG 700000
#define INIT_CAP 8

static const char* LEVELS[] = {
    "国家级", "省级", "地级", "县级", "乡级", "村级"
};

typedef struct {
    char code[LEN_CODE];
    char name[LEN_NAME];
    char p_code[LEN_CODE];
    int level;
    int type;
    union { double price; char* rate; } ext;
    char has_price;
} Region;

typedef struct node {
    Region data;
    struct node** kids;
    struct node* parent;
    int count;
    int cap;
} Node;

static Node* new_node(Region r) {
    Node* n = malloc(sizeof(Node));
    if (!n) return NULL;
    n->data = r;
    n->cap = INIT_CAP;
    if (!(n->kids = malloc(n->cap * sizeof(Node*)))) {
        free(n);
        return NULL;
    }
    n->count = 0;
    n->parent = NULL;
    return n;
}

static void add_kid(Node* p, Node* k) {
    if (p->count >= p->cap) {
        int new_cap = p->cap * 2;
        Node** new_kids = realloc(p->kids, new_cap * sizeof(Node*));
        if (!new_kids) return;
        p->kids = new_kids;
        p->cap = new_cap;
    }
    p->kids[p->count++] = k;
    k->parent = p;
}

static int valid(const char* s, int is_code) {
    if (!s || !*s) return 0;
    if (is_code) return strlen(s) == 12 && strspn(s, "0123456789") == 12;
    return strlen(s) < LEN_NAME && strspn(s, " \t\n") != strlen(s);
}

static void show(Node* n, int sep) {
    if (!n) return;
    if (sep) puts("---");
    
    printf("名称: %s\n代码: %s\n级别: %s\n", 
           n->data.name, n->data.code,
           (n->data.level >= 0 && n->data.level < 6) ? 
           LEVELS[n->data.level] : "未知");
    
    printf("%s: %s\n", n->data.has_price ? "房价" : "就业率",
           n->data.has_price ? 
           (n->data.ext.price > 0 ? "%.2f" : "暂无") :
           n->data.ext.rate);
    
    printf("层级: %s", n->data.name);
    for (Node* p = n->parent; p && p->data.level; p = p->parent)
        printf(" <- %s", p->data.name);
    putchar('\n');
}

static Node* find_code(Node* root, const char* code) {
    if (!root) return NULL;
    if (!strcmp(root->data.code, code)) return root;
    for (int i = 0; i < root->count; i++) {
        Node* found = find_code(root->kids[i], code);
        if (found) return found;
    }
    return NULL;
}

static void find_name(Node* root, const char* name, int* cnt) {
    if (!root || *cnt >= 5) return;
    if (strstr(root->data.name, name)) {
        show(root, *cnt > 0);
        if (++(*cnt) >= 5) {
            puts("\n仅显示前5条");
            return;
        }
    }
    for (int i = 0; i < root->count; i++)
        find_name(root->kids[i], name, cnt);
}

static int load(Region rs[], const char* file) {
    FILE* f = fopen(file, "r");
    if (!f) return 0;

    char line[LEN_LINE];
    int cnt = 0;
    fgets(line, LEN_LINE, f);
    
    while (fgets(line, LEN_LINE, f) && cnt < MAX_REG) {
        char* t = strtok(line, ",");
        if (!t) continue;
        
        Region* r = &rs[cnt];
        strncpy(r->code, t, LEN_CODE - 1);
        
        if (!(t = strtok(NULL, ","))) continue;
        strncpy(r->name, t, LEN_NAME - 1);
        
        if (!(t = strtok(NULL, ","))) continue;
        r->level = atoi(t);
        
        if (!(t = strtok(NULL, ","))) continue;
        strncpy(r->p_code, t, LEN_CODE - 1);
        
        if (!(t = strtok(NULL, ","))) continue;
        r->type = atoi(t);
        
        if ((t = strtok(NULL, ","))) {
            r->ext.price = atof(t);
            r->has_price = 1;
        } else if ((t = strtok(NULL, ","))) {
            r->ext.rate = strdup(t);
        } else {
            r->ext.rate = strdup("N/A");
        }
        cnt++;
    }
    fclose(f);
    return cnt;
}

static Node* build(Region rs[], int size) {
    Node** nodes = malloc(size * sizeof(Node*));
    struct { char code[LEN_CODE]; Node* node; } 
    *map = malloc(size * sizeof(*map));
    
    if (!nodes || !map) {
        free(nodes);
        free(map);
        return NULL;
    }

    printf("构建中...");
    for (int i = 0; i < size; i++) {
        if (!(nodes[i] = new_node(rs[i]))) {
            for (int j = 0; j < i; j++) {
                free(nodes[j]->kids);
                free(nodes[j]);
            }
            free(nodes);
            free(map);
            return NULL;
        }
        strncpy(map[i].code, rs[i].code, LEN_CODE - 1);
        map[i].node = nodes[i];
        if (!(i % 5000)) {
            printf("\r%d%%", (i + 1) * 100 / size);
            fflush(stdout);
        }
    }

    qsort(map, size, sizeof(*map), (int(*)(const void*,const void*))strcmp);

    Region root_data = {"000000000000", "中华人民共和国", "0", 0, 0, {0}, 0};
    Node* root = new_node(root_data);
    if (!root) {
        for (int i = 0; i < size; i++) {
            free(nodes[i]->kids);
            free(nodes[i]);
        }
        free(nodes);
        free(map);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        if (!strcmp(rs[i].p_code, "0")) {
            add_kid(root, nodes[i]);
            continue;
        }
        int l = 0, r = size - 1;
        while (l <= r) {
            int m = (l + r) / 2;
            int cmp = strcmp(map[m].code, rs[i].p_code);
            if (!cmp) {
                add_kid(map[m].node, nodes[i]);
                break;
            }
            if (cmp < 0) l = m + 1;
            else r = m - 1;
        }
    }
    puts("\n完成");
    free(map);
    free(nodes);
    return root;
}

static void free_tree(Node* root) {
    if (!root) return;
    for (int i = 0; i < root->count; i++)
        free_tree(root->kids[i]);
    if (!root->data.has_price)
        free(root->data.ext.rate);
    free(root->kids);
    free(root);
}

static int menu(Node* root) {
    static const char* m =
        "\n┌──────────┐\n"
        "│区划查询  │\n"
        "├──────────┤\n"
        "│1 代码    │\n"
        "│2 名称    │\n"
        "│3 退出    │\n"
        "└──────────┘\n"
        "\n[1-3]: ";
    
    char in[LEN_NAME];
    int c;
    
    while (1) {
        printf("%s", m);
        if (scanf("%d", &c) != 1) {
            while (getchar() != '\n');
            puts("请输入1-3");
            continue;
        }
        while (getchar() != '\n');

        switch (c) {
            case 1: {
                printf("\n代码: ");
                if (!fgets(in, LEN_NAME, stdin) || !valid(in, 1)) {
                    puts("代码无效");
                    continue;
                }
                in[strcspn(in, "\n")] = 0;
                puts("\n结果:");
                Node* f = find_code(root, in);
                if (f) show(f, 0);
                else printf("未找到: %s\n", in);
                break;
            }
            case 2: {
                printf("\n名称: ");
                if (!fgets(in, LEN_NAME, stdin) || !valid(in, 0)) {
                    puts("名称无效");
                    continue;
                }
                in[strcspn(in, "\n")] = 0;
                puts("\n结果:");
                int cnt = 0;
                find_name(root, in, &cnt);
                if (!cnt) printf("未找到: %s\n", in);
                else printf("\n共%d个匹配\n", cnt);
                break;
            }
            case 3: return 0;
            default: puts("请输入1-3");
        }
    }
}

int main() {
    Region* rs = malloc(MAX_REG * sizeof(Region));
    if (!rs) { puts("内存不足"); return 1; }

    puts("\n== 区划查询 ==");
    int size = load(rs, "area_code_2024_new.csv");
    if (!size) {
        puts("加载失败");
        free(rs);
        return 1;
    }

    printf("已加载%d条\n", size);
    Node* root = build(rs, size);
    if (!root) {
        puts("构建失败");
        free(rs);
        return 1;
    }

    int r = menu(root);
    free_tree(root);
    free(rs);
    return r;
} 