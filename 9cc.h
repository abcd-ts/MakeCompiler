// 9cc.h

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------
// -- tokenize.c --
// ----------------

// トークンの種類
typedef enum {
	TK_RESERVED,	// 記号
	TK_IDENT,		// 識別子
	TK_NUM,			// 整数トークン
	TK_EOF,			// 入力の終わり
	TK_RETURN,		// return
	TK_IF,
	TK_ELSE,
	TK_WHILE,
	TK_FOR,
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

// 引数のトークン種類と一致するトークンかどうか調べる
// 一致すればそれを返し読み進め，しなければNULLを返す
Token *consume_token(TokenKind kind);

// 入力の終わりかどうかを調べる
bool at_eof();

// 新しいトークンを生成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len);

// トークナイズする
// headというダミーの要素を用いる，headの次の要素を返す
void tokenize(char *p);

// -------------
// -- parse.c --
// -------------

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
	ND_RETURN,	// return
	ND_IF,
	ND_WHILE,
	ND_FOR,
	ND_BLOCK,
	ND_FUNCALL,	// 関数呼び出し
	ND_FUNC,	// 関数定義
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	
	Node *cond;	// 条件式
	Node *inc;	// for文の第3引数
	
	Node *next;	// block,関数の引数で使用するリストの次の要素
	
	int val;	// ND_NUMのときに使用
	int offset;	// ND_LVARのときに使用

	char *name;
	int len;
	Node *arg;

	Node *body;	// 関数の本体
};

// 新しいノードを作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);

// 新しい整数ノードを作成
Node *new_node_num(int val);

// stmtを格納
extern Node *code[];

// EBNDで示された規則に対応する関数

void program();
Node *def();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *primary();
Node *unary();

// Local Variables
typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
	LVar *next;	// 連結リストで表す
	char *name;
	int len;
	int offset;
};

// ローカル変数の連結リスト
extern LVar *locals;


typedef struct Func Func;

struct Func {
	Node *node;	// 関数ノード
	
	Func *next; // 次の関数
	LVar *locals;	// 関数のローカル変数
};

extern Func *functions;

// 名前で識別子を探す
LVar *find_lvar(Token *tok);

// ---------------
// -- codegen.c --
// ---------------

// nodeに対応するコードを生成
void codegen();
