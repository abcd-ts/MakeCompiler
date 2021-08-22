// parse.c
// トークナイズ，パースを行う

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// ----------------
// --トークナイザ---
// ----------------

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
	if (token->kind != TK_RESERVED || strlen(op) != token->len
		|| memcmp(token->str, op, token->len)) {
		//printf("token is not \"%s\"\n", op);
		return false;
	}
	token = token->next;
	return true;
}

// トークンを読み進める
// 期待している記号なら読み進めるだけで，そうでなければエラーを報告
void expect(char *op) {
if (token->kind != TK_RESERVED || strlen(op) != token->len
	|| memcmp(token->str, op, token->len)) {
		error_at(token->str, "\"%s\"ではありません", op);
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

// 識別子の消費を試みる
// 識別子ならトークンを読み進めて識別子トークンを返し，そうでなければNULLを返す
Token *consume_ident() {
	if (token->kind != TK_IDENT) {
		return NULL;
	}
	Token *tok = token;
	token = token->next;
	return tok;
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
void tokenize(char *p) {
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
		if (strchr("+-*/()<>=;", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		// number
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		// identifier
		if (isalpha(*p)) {
			char *start = p;
			while (isalpha(*p)) {
				p++;
			}
			cur = new_token(TK_IDENT, cur, start, p - start);
			continue;
		}

		error_at(token->str, "tokenizeできません");
	}

	new_token(TK_EOF, cur, p, 0);
	token = head.next;
}

// ------------
// --構文解析---
// ------------

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

Node *code[100];

// program = stmt*
void program() {
	int i = 0;

	while (!at_eof()) {
		code[i++] = stmt();
	}
	code[i] == NULL;	// 末尾を示す
}

// stmt = expr ';'
Node *stmt() {
	Node *node = expr();
	expect(";");
	return node;
}

// expr = assign
Node *expr() {
	return assign();
}

// assign = equality ("=" assign)?
Node *assign() {
	Node *node = equality();

	if (consume("=")) {
		node = new_node(ND_ASSIGN, node, assign());
	}
	return node;
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

// primary = num | ident | '(' expr ')'
Node *primary() {
	// '(' expr ')'
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	// ident
	Token *tok = consume_ident();

	if (tok) {
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;

		LVar *lvar = find_lvar(tok);
		if (lvar) {	// 既存の変数
			node->offset = lvar->offset;
		}
		else { // 新たな変数
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;
			if (locals) {
				lvar->offset = locals->offset + 8;
			}
			else {
				lvar->offset = 8;
			}
			node->offset = lvar->offset;
			locals = lvar;
		}
		return node; 
	}

	// num
	return new_node_num(expect_number());
}

// ローカル変数の連結リスト
LVar *locals = NULL;

// 変数を名前で検索する．見つからなければNULLを返す
LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}
