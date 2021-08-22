// 9cc.h

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
	TK_IDENT,		// 識別子
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
extern Token *token;

void error(char *fmt, ...);

// 入力
extern char *user_input;

// エラー箇所報告
void error_at(char *loc, char *fmt, ...);

// トークンを読み進める
// 期待している記号なら真，そうでなければ偽
bool consume(char *op);

// トークンを読み進める
// 期待している記号なら読み進めるだけで，そうでなければエラーを報告
void expect(char *op);

// 次のトークンが数値である場合，トークンを読み進めてその値を返す．
// 数値でない場合，エラーを報告する．
int expect_number();

Token *consume_ident();

// 入力の終わりかどうかを調べる
bool at_eof();

// 新しいトークンを生成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len);

// トークナイズする
// headというダミーの要素を用いる，headの次の要素を返す
void tokenize(char *p);

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
	ND_EQ,	// ==
	ND_NEQ,	// !=
	ND_LEQ,	// <=
	ND_LE,	// <
	ND_ASSIGN,	// =
	ND_LVAR,	// ローカル変数
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;	// ND_NUMのときに使用
	int offset;	// ND_LVARのときに使用
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);

Node *new_node_num(int val);

extern Node *code[];

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *primary();
Node *unary();

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
	LVar *next;	// 連結リストで表す
	char *name;
	int len;
	int offset;
};

extern LVar *locals;

LVar *find_lvar(Token *tok);

// ---------------
// -- コード生成 --
// ---------------

void gen(Node *node);

void gen_lval(Node *node);
