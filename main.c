// main.c
// main関数

#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません");
        return 1;
    }

	// 入力を覚えておく
	user_input = argv[1];
	
	// トークナイズする
	token = tokenize(user_input);

	// パースする
	Node *node = expr();

	// アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

	// ASTからコード生成
	gen(node);

	// スタックトップの値が答え
	printf("    pop rax\n");
	printf("    ret\n");
	return 0;
}
