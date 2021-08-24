// parse.c
// パースを行う

#include "9cc.h"

// 新しいノードを作成する
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

// 新しい整数ノードを作成する
Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

// defを保存する
Node *code[100];

// program = def*
void program() {
	int i = 0;

	while (!at_eof()) {
		code[i++] = def();
	}
	code[i] == NULL;	// 末尾を示す
}

// def = ident '(' expr* ')' '{' stmt* '}'
Node *def() {
	// ident ("(" ")")?
	Token *tok = consume_token(TK_IDENT);

	if (!tok) {
		error("識別子ではありません");
	}

	Node *node = calloc(1, sizeof(Node));
	expect("(");
	node->kind = ND_FUNC;
	node->name = tok->str;
	node->len = tok->len;

	// 引数

	expect(")");
	
	expect("{");
	
	Node *block_head = calloc(1, sizeof(Node));
	block_head->kind = ND_BLOCK;
	Node *cur = block_head;
	while (!consume("}") || !at_eof()) {
		cur->next = stmt();
		cur = cur->next;
	}
	cur->next = NULL;

	node->body = block_head;

	//Node head;
	//head.next = NULL;
	//Node *arg = &head;
	//while (!consume(")") && !at_eof()) {
	//	arg->next = expr();
	//	arg = arg->next;
	//	if (!consume(",")) {
	//		expect(")");
	//		break;
	//	}
	//}
	//node->arg = head.next;
	
	return node;
}

/*
	stmt = expr ';' | "return" expr ';'
		 | "if (" expr ")" stmt ("else" stmt)? 
		 | "while (" expr ")" stmt
		 | "for (" expr? ";" expr? ";" expr? ")" stmt
		 | "{" stmt* "}"
*/
Node *stmt() {
	Node *node;

	if (consume_token(TK_RETURN)) {	// return文
		node = calloc(1, sizeof(Node));
		node->kind = ND_RETURN;
		node->lhs = expr();
	}
	else if (consume_token(TK_IF)) {	// if文
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_IF;
		node->cond = expr();
		expect(")");
		node->lhs = stmt();

		if (consume_token(TK_ELSE)) {
			node->rhs = stmt();
		}
		else {
			node->rhs = NULL;
		}
		return node;
	}
	else if (consume_token(TK_WHILE)) {
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_WHILE;
		node->cond = expr();
		expect(")");
		node->lhs = stmt();

		return node;
	}
	else if (consume_token(TK_FOR)) {
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_FOR;
		if (consume(";")) {
			node->lhs = NULL;
		}
		else {
			node->lhs = expr();
			expect(";");
		}

		if (consume(";")) {
			node->cond = NULL;
		}
		else {
			node->cond = expr();
			expect(";");
		}

		if (!consume(")")) {
			node->inc = expr();
			expect(")");
		}
		else {
			node->inc = NULL;
		}
		
		node->rhs = stmt();
		
		return node;
	}
	else if (consume("{")) {
		Node *head = calloc(1, sizeof(Node));
		head->kind = ND_BLOCK;
		Node *cur = head;
		while (!consume("}") || !at_eof()) {
			cur->next = stmt();
			cur = cur->next;
		}
		cur->next = NULL;
		return head;
	}
	else {
		node = expr();
	}

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

// primary = num 
//		   | ident ("(" expr* ")")?
//		   | '(' expr ')'
Node *primary() {
	// '(' expr ')'
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	// ident ("(" ")")?
	Token *tok = consume_token(TK_IDENT);

	if (tok) {
		Node *node = calloc(1, sizeof(Node));
		if (consume("(")) {	// 関数呼び出し
			node->kind = ND_FUNCALL;
			node->name = tok->str;
			node->len = tok->len; 

			Node head;
			head.next = NULL;
			Node *arg = &head;
			while (!consume(")") && !at_eof()) {
				arg->next = expr();
				arg = arg->next;
				if (!consume(",")) {
					expect(")");
					break;
				}
			}
			node->arg = head.next;
			
			return node;
		}
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
