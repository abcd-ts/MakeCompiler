// codegen.c

#include "9cc.h"

static int num_end = 0;
static int num_else = 0;
static int num_begin = 0;

static int depth;

static void push(char *arg) {
	printf("    push %s\n", arg);
	depth++;
}

static void push_num(int n) {
	printf("    push %d\n", n);
	depth++;
}

static void pop(char *arg) {
	printf("    pop %s\n", arg);
	depth--;
}

static void gen_stmt(Node *node);
static void gen_expr(Node *node);

// ローカル変数に対応するコードを生成
static void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) {
		error("代入の左辺値が不正です");
	}

	printf("    mov rax, rbp\n");	// raxにベースポインタの値を読み込む
	printf("    sub rax, %d\n", node->offset);	// raxにオフセットを減算
	push("rax");
}

// if文に対応するコードを生成
static void gen_if(Node *node) {
	int lend = num_end++, lelse = num_else++;

	gen_expr(node->cond);
	// スタックトップに条件式の結果
	pop("rax");
	printf("    cmp rax, 0\n");
	if (!node->rhs) {	// elseなし
		printf("    je .Lend%d\n", lend);
		gen_stmt(node->lhs);
		printf(".Lend%d:\n", lend);
	}
	else {	// elseあり
		printf("    je .Lelse%d\n", lelse);
		gen_stmt(node->lhs);
		printf("    jmp .Lend%d\n", lend);
		printf(".Lelse%d:\n", lelse);
		gen_stmt(node->rhs);
		printf(".Lend%d:\n", lend);
	}
}

// while文のコードを生成
static void gen_while(Node *node) {
	int lbegin = num_begin++, lend = num_end++;

	printf(".Lbegin%d:\n", lbegin);
	gen_expr(node->cond);
	pop("rax");
	printf("    cmp rax, 0\n");
	printf("    je .Lend%d\n", lend);
	gen_stmt(node->lhs);
	printf("    jmp .Lbegin%d\n", lbegin);
	printf(".Lend%d:\n", lend);
}

// for文のコードを生成
static void gen_for(Node *node) {
	int lbegin = num_begin++, lend = num_end++;

	if (node->lhs) {
		gen_expr(node->lhs);
	}

	printf(".Lbegin%d:\n", lbegin);
	if (node->cond) {
		gen_expr(node->cond);
		pop("rax");
		printf("    cmp rax, 0\n");
		printf("    je .Lend%d\n", lend);
	}
	gen_stmt(node->rhs);
	if (node->inc) {
		gen_expr(node->inc);
	}
	printf("    jmp .Lbegin%d\n", lbegin);
	printf(".Lend%d:\n", lend);
}

// blockのコードを生成
static void gen_block(Node *node) {
	Node *cur = node->next;
	while (cur) {
		gen_stmt(cur);
		pop("rax");
		cur = cur->next;
	}
}

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// 関数呼び出しのコードを生成
static void gen_func(Node *node) {
	Node *cur = node->arg;
	int i, n = 0;
	while (cur) {
		gen_expr(cur);
		cur = cur->next;
		n++;
	}
	for (i = 0; i < n; i++) {
		pop(argreg[i]);
	}
	// rspを16の倍数に
	printf("    mov rbx, 0x000F\n");
	printf("    and rbx, rsp\n");
	printf("    cmp rbx, 0\n");
	printf("    je .L%.*s\n", node->len, node->name);
	printf("    sub rsp, 8\n");
	printf(".L%.*s:\n", node->len, node->name);

	printf("    call %.*s\n", node->len, node->name);
}

// expr
static void gen_expr(Node *node) {
	// 整数またはローカル変数，代入式，return文
	switch (node->kind) {
	case ND_NUM:
		push_num(node->val);
		return;
	case ND_LVAR:	// ローカル変数の場合，アドレスから値を取り出す
		gen_lval(node);
		pop("rax");
		printf("    mov rax, [rax]\n");
		push("rax");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen_expr(node->rhs);

		pop("rdi");
		pop("rax");
		printf("    mov [rax], rdi\n");
		push("rdi");
		return;
	case ND_FUNC:
		gen_func(node);
		return;
	}

	// ASTを帰りがけ順で走査
	gen_expr(node->lhs);
	gen_expr(node->rhs);

	// 結果がスタックのトップから2つに入っている
	pop("rdi");
	pop("rax");

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
	// 演算結果をスタックにプッシュ
	push("rax");
}

// stmt
static void gen_stmt(Node *node) {
	int lbegin, lend, lelse;
	switch (node->kind) {
	case ND_RETURN:
		gen_expr(node->lhs);
		// 結果がスタックトップに入っている
		printf("    pop rax\n");
		printf("    mov rsp, rbp\n");
		printf("    pop rbp\n");
		printf("    ret\n");
		return;
	case ND_IF:
		gen_if(node);
		return;
	case ND_WHILE:
		gen_while(node);
		return;
	case ND_FOR:
		gen_for(node);
		return;
	case ND_BLOCK:
		gen_block(node);
		return;
	default:
		gen_expr(node);
	}
}

void codegen() {
	depth = 0;

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
		gen_stmt(code[i]);

		// 1つの式の評価結果がスタックに残っているため
		// これをpopしてraxに入れておく
		// blockの場合毎回popしているため，ここではpopしない
		if (depth) pop("rax");
	}

	// epilogue
	// スタックトップの値が答え
	printf("    mov rsp, rbp\n");
	printf("    pop rbp\n");
	printf("    ret\n");
}