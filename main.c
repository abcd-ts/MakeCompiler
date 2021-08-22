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

	// トークナイズしてパースする
	// 結果(AST)はcodeに保存される
	tokenize(user_input);
	program();

	codegen();
	
	return 0;
}
