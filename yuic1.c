#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void writeStart()
{
    puts(".intel_syntax noprefix");
    puts(".global main");
    puts("main:");
    puts("  push rbp");
    puts("  mov rbp, rsp");
    puts("  sub rsp, 16");
    puts("  mov DWORD PTR [rbp-4], 0");
    puts("  mov esi, 1");
    puts("  mov edi, 256");
    puts("  call calloc");
    puts("  mov QWORD PTR [rbp-16], rax");
}

void writeEnd()
{
    puts("  mov eax, 0");
    puts("  leave");
    puts("  ret");
}

// brainf*ck との対応関係
typedef struct
{
    int bf;
    char *basicn;
} Conv;

static Conv st_conv[] = {
    {'>', "駆け出せ迷わずに"},
    {'<', "踏み出せ迷わずに"},
    {'+', "いっせーのーで"},
    {'-', "もう少し眠りたいけど"},
    {'.', "君とだけ!"},
    {',', "ハイタッチ"},
    {'[', "巡り巡る日々を辿り夢見たステージへ"},
    {']', "めくりめく季節君と次のステージへ"},
};

typedef struct
{
    int bf_code;
    int address;
} BfCode;
// 入力を格納する配列
static BfCode st_bf_code[10000];
static int st_code_count;

static int
compare_conv(const void *a, const void *b)
{
    return strlen(((Conv *)b)->basicn) - strlen(((Conv *)a)->basicn);
}

static void
sort_conv_table(Conv *table, int table_count)
{
    qsort(table, table_count, sizeof(Conv), compare_conv);
}

static void
read_source(FILE *fp)
{
    unsigned char line[4096];
    int i;
    int j;
    int code_idx = 0;
    int stack[256];
    int sp = 0;

    while ((fgets(line, 4096, fp)) != NULL)
    {
        for (i = 0; line[i] != '\0';)
        {
            if (isspace(line[i]))
            {
                i++;
                continue;
            }
            for (j = 0; j < sizeof(st_conv) / sizeof(Conv); j++)
            {
                int len = strlen(st_conv[j].basicn);
                if (!strncmp(&line[i], st_conv[j].basicn, len))
                {
                    st_bf_code[code_idx].bf_code = st_conv[j].bf;

                    if (st_conv[j].bf == '[')
                    {
                        stack[sp] = code_idx;
                        sp++;
                    }
                    else if (st_conv[j].bf == ']')
                    {
                        st_bf_code[stack[sp - 1]].address = code_idx;
                        st_bf_code[code_idx].address = stack[sp - 1];
                        sp--;
                        if (sp < 0)
                        {
                            fprintf(stderr, "bracket mismatch\n");
                            exit(2);
                        }
                    }
                    code_idx++;
                    i += len;
                    break;
                }
            }
            if (j == sizeof(st_conv) / sizeof(Conv))
            {
                i++;
            }
        }
    }
    if (sp != 0)
    {
        fprintf(stderr, "bracket mismatch\n");
        exit(2);
    }

    st_code_count = code_idx;
}

static void
writeAssem(void)
{
    int array[65536];
    int p = 0;
    int labelIndex;
    int labelLayer;
    int labelLayers[100];
    labelIndex = 0;
    labelLayer = 0;

    for (int i = 0; i < st_code_count; i++)
    {
        switch (st_bf_code[i].bf_code)
        {
        // ポインタの指す値を増やす
        case '+':
            puts("  mov eax, DWORD PTR [rbp-4]");
            puts("  movsx rdx, eax");
            puts("  mov rax, QWORD PTR [rbp-16]");
            puts("  add rax, rdx");
            puts("  movzx edx, BYTE PTR [rax]");
            puts("  add edx, 1");
            puts("  mov BYTE PTR [rax], dl");
            break;
        // ポインタの指す値を増やす
        case '-':
            puts("  mov eax, DWORD PTR [rbp-4]");
            puts("  movsx rdx, eax");
            puts("  mov rax, QWORD PTR [rbp-16]");
            puts("  add rax, rdx");
            puts("  movzx edx, BYTE PTR [rax]");
            puts("  sub edx, 1");
            puts("  mov BYTE PTR [rax], dl");
            break;
        // ポインタを増やす
        case '>':
            puts("  add DWORD PTR [rbp-4], 1");
            break;
        // ポインタを減らす
        case '<':
            puts("  sub QWORD PTR [rbp-4], 1");
            break;
        // 出力
        case '.':
            puts("  mov eax, DWORD PTR [rbp-4]");
            puts("  movsx rdx, eax");
            puts("  mov rax, QWORD PTR [rbp-16]");
            puts("  add rax, rdx");
            puts("  movzx eax, BYTE PTR [rax]");
            puts("  movsx eax, al");
            puts("  mov edi, eax");
            puts("  call putchar");
            break;
        //　初期化
        case ',':
            puts("  mov eax, DWORD PTR [rbp-4]");
            puts("  movsx rdx, eax");
            puts("  mov rax, QWORD PTR [rbp-16]");
            puts("  add rax, rdx");
            puts("  mov BYTE PTR [rax], 97");
            break;
        case '[':
            labelLayers[labelLayer] = labelIndex;
            printf("  jmp .A%d\n", labelLayers[labelLayer]);
            printf(".B%d:\n", labelLayers[labelLayer]);
            labelIndex++;
            labelLayer++;
            break;
        case ']':
            labelLayer--;
            printf(".A%d:\n", labelLayers[labelLayer]);
            puts("  mov rax, QWORD PTR [rbp-16]");
            puts("  movzx eax, BYTE PTR [rax]");
            puts("  test al, al");
            printf("  jne .B%d\n", labelLayers[labelLayer]);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char **argv)
{
    FILE *in_fp;

    sort_conv_table(st_conv, sizeof(st_conv) / sizeof(Conv));

    if (argc == 2)
    {
        in_fp = fopen(argv[1], "r");
        if (in_fp == NULL)
        {
            fprintf(stderr, "%s not found.\n", argv[1]);
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "Usage:%s filename\n", argv[0]);
        exit(1);
    }

    read_source(in_fp);
    writeStart();
    writeAssem();
    writeEnd();

    return 0;
}