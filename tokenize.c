// tokenize.c
// トークナイズを行う

#include "9cc.h"

// トークン列
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

// "return"の消費を試みる
// returnトークンなら読み進めてそれを返し，そうでなければNULLを返す
Token *consume_return() {
	if (token->kind != TK_RETURN) {
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

// 英数字またはアンダースコアかどうかを判定する
// そうなら正の値，それ以外なら0を返す
int is_alnum(char c) {
	return ('a' <= c && c <= 'z') ||
		   ('A' <= c && c <= 'Z') ||
		   ('0' <= c && c <= '9') ||
		   (c == '_');
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

		// return
		if (strncmp(p, "return", 6) == 0&& !is_alnum(*(p + 6))) {
			cur = new_token(TK_RETURN, cur, p, 6);
			p += 6;
			continue;
		}

		// identifier
		if (isalpha(*p) || *p == '_') {
			char *start = p;
			while (isalnum(*p)) {
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
