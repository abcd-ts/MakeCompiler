#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------
// --トークナイザ---
// ----------------

// トークンの種類
typedef enum {
	TK_RESERVED,	// 記号
	TK_NUM,			// 整数トークン
	TK_EOF,			// 入力の終わり
} TokenKind;

typedef struct Token Token;

// Token型
// 連結リストでトークン列を表現する
struct Token {
	TokenKind kind;
	Token *next;
	int val;	// TK_NUMの場合の数値
	char *str;	// トークン文字列
	int len; 	// トークンの長さ
};

// 現在着目しているトークン
// 特定の関数(consume, expect, expect_number)以外では触らない
Token *token;

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// 入力
char *user_input;

// エラー箇所報告
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// トークンを読み進める
// 期待している記号なら真，そうでなければ偽
bool consume(char *op) {
	if (token->kind != TK_RESERVED
		|| strlen(op) != token->len
		|| memcmp(token->str, op, token->len)) {
		return false;
	}
	token = token->next;
	return true;
}

// トークンを読み進める
// 期待している記号なら読み進めるだけで，そうでなければエラーを報告
void expect(char *op) {
if (token->kind != TK_RESERVED
		|| strlen(op) != token->len
		|| memcmp(token->str, op, token->len)) {
		error_at(token->str, "'%cではありません", op);
	}
	token = token->next;
}

// 次のトークンが数値である場合，トークンを読み進めてその値を返す．
// 数値でない場合，エラーを報告する．
int expect_number() {
	if (token->kind != TK_NUM) {
		error_at(token->str, "数ではありません");
	}
	int val = token->val;
	token = token->next;
	return val;
}

// 入力の終わりかどうかを調べる
bool at_eof() {
	return token->kind == TK_EOF;
}

// 新しいトークンを生成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

// トークナイズする
// headというダミーの要素を用いる，headの次の要素を返す
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// 空白文字
		if (isspace(*p)) {
			p++;
			continue;
		}

		// 長さ2の記号
		if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0
			|| strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}

		// 長さ1の記号
		if (strchr("+-*/()<>", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0); // 整数トークンの長さはとりあえず0にしておく
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(token->str, "tokenizeできません");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}

// -----------------------------

// ------------
// --構文解析---
// ------------

// ASTのノードの種類
typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NUM,
	ND_EQ,
	ND_NEQ,
	ND_LEQ,
	ND_LE,
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *primary();
Node *unary();

// expr = equality
Node *expr() {
	return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
	Node *node = relational();

	for (;;) {
		if (consume("==")) {
			node = new_node(ND_EQ, node, relational());
		}
		else if (consume("!=")) {
			node = new_node(ND_NEQ, node, relational());
		}
		else {
			return node;
		}
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
	Node *node = add();

	for (;;) {
		if (consume("<")) {
			node = new_node(ND_LE, node, add());
		}
		else if (consume("<=")) {
			node = new_node(ND_LEQ, node, add());
		}
		else if (consume(">")) {
			node = new_node(ND_LE, add(), node);
		}
		else if (consume(">=")) {
			node = new_node(ND_LEQ, add(), node);
		}
		else {
			return node;
		}
	}
}

// add = mul ('+' mul | '-' mul)*
Node *add() {
	Node *node = mul();

	for (;;) {
		if (consume("+")) {
			node = new_node(ND_ADD, node, mul());
		}
		else if (consume("-")) {
			node = new_node(ND_SUB, node, mul());
		}
		else {
			return node;
		}
	}
}

// mul = unary ('*' unary | '/' unary)*
Node *mul() {
	Node *node = unary();

	for (;;) {
		if (consume("*")) {
			node = new_node(ND_MUL, node, unary());
		}
		else if (consume("/")) {
			node = new_node(ND_DIV, node, unary());
		}
		else {
			return node;
		}
	}
}

// unary = ('+'|'-')? primary
Node *unary() {
	if (consume("+")) {
		return primary();
	}
	if (consume("-")) {
		return new_node(ND_SUB, new_node_num(0), primary());
	}
	return primary();
}

// primary = num | '(' expr ')'
Node *primary() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	return new_node_num(expect_number());
}

// ---------------
// -- コンパイル --
// ---------------

void gen(Node *node) {
	if (node->kind == ND_NUM) {
		printf("    push %d\n", node->val);
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
