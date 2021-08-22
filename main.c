// main.c
// main関数

#include <stdio.h>

#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません");
        return 1;
    }

	// 入力を覚えておく
	user_input = argv[1];

	// トークナイズしてパースする
	// 結果(AST)はcodeに保存される
	tokenize(user_input);
	program();

	// アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

	// prologue
	// 変数26個(a~z)分の領域を確保する
	printf("    push rbp\n");
	printf("    mov rbp, rsp\n");
	printf("    sub rsp, 208\n");

	// ASTからコード生成
	int i;
	for (i = 0; code[i] != NULL; i++) {
		gen(code[i]);

		// 1つの式の評価結果がスタックに残っているため
		// これをpopしてraxに入れておく
		printf("    pop rax\n");
	}

	// epilogue
	// スタックトップの値が答え
	printf("    mov rsp, rbp\n");
	printf("    pop rbp\n");
	printf("    ret\n");
	return 0;
}
