// codegen.c

#include "9cc.h"

// ---------------
// -- コード生成 --
// ---------------

void gen(Node *node) {
	// 整数またはローカル変数，代入式，return文
	switch (node->kind) {
	case ND_NUM:
		printf("    push %d\n", node->val);
		return;
	case ND_LVAR:	// ローカル変数の場合，アドレスから値を取り出す
		gen_lval(node);
		printf("    pop rax\n");
		printf("    mov rax, [rax]\n");
		printf("    push rax\n");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);

		printf("    pop rdi\n");
		printf("    pop rax\n");
		printf("    mov [rax], rdi\n");
		printf("    push rdi\n");
		return;
	case ND_RETURN:
		gen(node->lhs);
		
		printf("    pop rax\n");
		printf("    mov rsp, rbp\n");
		printf("    pop rbp\n");
		printf("    ret\n");
		return;
	}

	// ASTを帰りがけ順で走査
	gen(node->lhs);
	gen(node->rhs);

	printf("    pop rdi\n");
	printf("    pop rax\n");

	switch (node->kind) {
	case ND_ADD:
		printf("    add rax, rdi\n");
		break;
	case ND_SUB:
		printf("    sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("    imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("    cqo\n");
		printf("    idiv rdi\n");
		break;
	case ND_EQ:
		printf("    cmp rax, rdi\n");
		printf("    sete al\n");
		printf("    movzb rax, al\n");
		break;
	case ND_NEQ:
		printf("    cmp rax, rdi\n");
		printf("    setne al\n");
		printf("    movzb rax, al\n");
		break;
	case ND_LE:
		printf("    cmp rax, rdi\n");
		printf("    setl al\n");
		printf("    movzb rax, al\n");
		break;
	case ND_LEQ:
		printf("    cmp rax, rdi\n");
		printf("    setle al\n");
		printf("    movzb rax, al\n");
		break;
	}

	printf("    push rax\n");
}

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) {
		error("代入の左辺値が不正です");
	}

	printf("    mov rax, rbp\n");	// raxにベースポインタの値を読み込む
	printf("    sub rax, %d\n", node->offset);	// raxにオフセットを減算
	printf("    push rax\n");
}
