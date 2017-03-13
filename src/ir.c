/*
 * ir.c
 * 3-point IR instructions.
 */

#include <string.h>

#include "ir.h"
#include "types.h"

void ir_init(struct ir_sequence *ir)
{
	vector_init(&ir->seq, sizeof (struct ir_instruction));
}

void ir_destroy(struct ir_sequence *ir)
{
	vector_destroy(&ir->seq);
}

void ir_clear(struct ir_sequence *ir)
{
	vector_clear(&ir->seq);
}

#define IS_TERM(n) \
	((n)->tag == NODE_CONSTANT \
	 || (n)->tag == NODE_IDENTIFIER \
	 || (n)->tag == NODE_STRLIT)

struct tmp_reg {
	int next;
	int items[NUM_TEMP_REGS];
};

static int ir_read_ast(struct ir_sequence *ir,
                       struct ast_node *expr,
                       struct tmp_reg *temps);

static int ir_parse_lvalue_deref(struct ir_sequence *ir,
                                 struct ast_node *expr,
                                 struct tmp_reg *temps)
{
	struct ir_instruction inst;
	int deref, tmpreg;

	deref = 0;
	for (; expr->tag == EXPR_DEREFERENCE; expr = expr->left)
		++deref;

	if (IS_TERM(expr)) {
		tmpreg = temps->next;
		temps->next = temps->items[temps->next];

		inst.tag = IR_LOAD;
		inst.target = tmpreg;
		inst.type_flags = expr->expr_flags;
		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = expr;
		vector_append(&ir->seq, &inst);
	} else {
		tmpreg = ir_read_ast(ir, expr, temps);
	}

	for (; deref > 1; --deref) {
		inst.tag = EXPR_DEREFERENCE;
		inst.target = tmpreg;
		inst.lhs.op_type = IR_OPERAND_TEMP_REG;
		inst.lhs.reg = tmpreg;
		vector_append(&ir->seq, &inst);
	}

	return tmpreg;
}

/*
 * ir_parse_arguments:
 * Parse the argument list for a function and convert to IR instructions.
 */
static void ir_parse_arguments(struct ir_sequence *ir,
                               struct ast_node *arglist,
                               struct tmp_reg *temps)
{
	struct ir_instruction inst;

	if (!arglist)
		return;

	if (IS_TERM(arglist)) {
		inst.tag = IR_PUSH;
		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = arglist;
	} else if (arglist->tag == EXPR_COMMA) {
		ir_parse_arguments(ir, arglist->right, temps);
		ir_parse_arguments(ir, arglist->left, temps);
		return;
	} else {
		/* Some expression. */
		inst.tag = IR_PUSH;
		inst.lhs.op_type = IR_OPERAND_TEMP_REG;
		inst.lhs.reg = ir_read_ast(ir, arglist, temps);
		temps->items[inst.lhs.reg] = temps->next;
		temps->next = inst.lhs.reg;
	}
	vector_append(&ir->seq, &inst);
}

static int ir_read_ast(struct ir_sequence *ir,
                       struct ast_node *expr,
                       struct tmp_reg *temps)
{
	struct ir_instruction inst;
	int tmp;

	inst.tag = expr->tag;
	inst.type_flags = expr->expr_flags;

	if (expr->tag == EXPR_FUNC) {
		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = expr->left;
		inst.rhs.op_type = IR_OPERAND_AST_NODE;
		inst.rhs.node = expr->right;
		ir_parse_arguments(ir, expr->right, temps);

		inst.target = temps->next;
		temps->next = temps->items[temps->next];

		vector_append(&ir->seq, &inst);
		return inst.target;
	}

	if (!expr->right) {
		/* Unary operator. */
		if (IS_TERM(expr->left)) {
			inst.lhs.op_type = IR_OPERAND_AST_NODE;
			inst.lhs.node = expr->left;

			inst.target = temps->next;
			temps->next = temps->items[temps->next];
		} else {
			inst.lhs.op_type = IR_OPERAND_TEMP_REG;
			inst.lhs.reg = ir_read_ast(ir, expr->left, temps);
			inst.target = inst.lhs.reg;
		}

		memset(&inst.rhs, ~0, sizeof inst.rhs);
		vector_append(&ir->seq, &inst);
		return inst.target;
	}

	if (expr->tag == EXPR_COMMA) {
		if (!IS_TERM(expr->left)) {
			tmp = ir_read_ast(ir, expr->left, temps);
			/* Discard the result. */
			temps->items[tmp] = temps->next;
			temps->next = tmp;
		}
		if (IS_TERM(expr->right)) {
			/* Bit of a hack, but no one does this in C anyway. */
			inst.tag = EXPR_UNARY_PLUS;
			inst.target = temps->next;
			temps->next = temps->items[temps->next];
			inst.lhs.op_type = IR_OPERAND_AST_NODE;
			inst.lhs.node = expr->right;
			vector_append(&ir->seq, &inst);
			return inst.target;
		} else {
			return ir_read_ast(ir, expr->right, temps);
		}
	}

	if (IS_TERM(expr->left) && IS_TERM(expr->right)) {
		/* Two terminal values: need a new temporary register. */
		inst.target = temps->next;
		temps->next = temps->items[temps->next];

		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = expr->left;
		inst.rhs.op_type = IR_OPERAND_AST_NODE;
		inst.rhs.node = expr->right;
	} else if (IS_TERM(expr->left)) {
		/* Expression and constant: update expression's register. */
		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = expr->left;
		inst.rhs.op_type = IR_OPERAND_TEMP_REG;
		inst.rhs.reg = ir_read_ast(ir, expr->right, temps);

		inst.target = inst.rhs.reg;
	} else if (IS_TERM(expr->right)) {
		/* Ditto. */
		inst.lhs.op_type = IR_OPERAND_TEMP_REG;
		if (expr->tag == EXPR_ASSIGN &&
		    expr->left->tag == EXPR_DEREFERENCE)
			inst.lhs.reg = ir_parse_lvalue_deref(ir, expr->left,
			                                     temps);
		else
			inst.lhs.reg = ir_read_ast(ir, expr->left, temps);

		inst.rhs.op_type = IR_OPERAND_AST_NODE;
		inst.rhs.node = expr->right;

		inst.target = inst.lhs.reg;
	} else {
		/* Both operands are expressions, use left's register. */
		inst.lhs.op_type = IR_OPERAND_TEMP_REG;
		if (expr->tag == EXPR_ASSIGN &&
		    expr->left->tag == EXPR_DEREFERENCE)
			inst.lhs.reg = ir_parse_lvalue_deref(ir, expr->left,
			                                     temps);
		else
			inst.lhs.reg = ir_read_ast(ir, expr->left, temps);

		inst.rhs.op_type = IR_OPERAND_TEMP_REG;
		inst.rhs.reg = ir_read_ast(ir, expr->right, temps);

		inst.target = inst.lhs.reg;

		/* We no longer need the value in temp register rhs. */
		temps->items[inst.rhs.reg] = temps->next;
		temps->next = inst.rhs.reg;
	}

	vector_append(&ir->seq, &inst);
	return inst.target;
}

static void ir_compare_zero(struct ir_sequence *ir, int term, void *item)
{
	struct ir_instruction inst;

	inst.tag = IR_TEST;
	inst.target = -1;
	inst.type_flags = TYPE_INT;
	if (term) {
		inst.lhs.op_type = IR_OPERAND_AST_NODE;
		inst.lhs.node = item;
	} else {
		inst.lhs.op_type = IR_OPERAND_TEMP_REG;
		inst.lhs.reg = ((struct ir_instruction *)item)->target;
	}
	vector_append(&ir->seq, &inst);
}

void ir_parse_expr(struct ir_sequence *ir, struct ast_node *expr, int cond)
{
	struct tmp_reg t;
	struct ir_instruction inst;
	int i;

	if (expr->tag == NODE_STRLIT)
		return;

	t.next = 0;
	for (i = 0; i < NUM_TEMP_REGS - 1; ++i)
		t.items[i] = i + 1;
	t.items[i] = -1;

	if (cond && !TAG_IS_COND(expr->tag)) {
		if (expr->tag == NODE_CONSTANT || expr->tag == NODE_IDENTIFIER) {
			ir_compare_zero(ir, 1, expr);
		} else {
			ir_read_ast(ir, expr, &t);
			vector_get(&ir->seq, ir->seq.nmembs - 1, &inst);
			ir_compare_zero(ir, 0, &inst);
		}
		return;
	}

	ir_read_ast(ir, expr, &t);
}

static void ir_print_operand(struct ir_operand *op)
{
	if (op->op_type == IR_OPERAND_TEMP_REG) {
		printf("t%d", op->reg);
		return;
	}

	if (op->node->tag == NODE_CONSTANT)
		printf("%ld", op->node->value);
	else
		printf("%s", op->node->lexeme);
}

static char *expr_str[] = {
	[EXPR_COMMA]            = ",",
	[EXPR_ASSIGN]           = "=",
	[EXPR_LOGICAL_OR]       = "||",
	[EXPR_LOGICAL_AND]      = "&&",
	[EXPR_OR]               = "|",
	[EXPR_XOR]              = "^",
	[EXPR_AND]              = "&",
	[EXPR_EQ]               = "==",
	[EXPR_NE]               = "!=",
	[EXPR_LT]               = "<",
	[EXPR_GT]               = ">",
	[EXPR_LE]               = "<=",
	[EXPR_GE]               = ">=",
	[EXPR_ADD]              = "+",
	[EXPR_SUB]              = "-",
	[EXPR_LSHIFT]           = "<<",
	[EXPR_RSHIFT]           = ">>",
	[EXPR_MULT]             = "*",
	[EXPR_DIV]              = "/",
	[EXPR_MOD]              = "%",
	[EXPR_ADDRESS]          = "&",
	[EXPR_DEREFERENCE]      = "*",
	[EXPR_UNARY_PLUS]       = "+",
	[EXPR_UNARY_MINUS]      = "-",
	[EXPR_NOT]              = "~",
	[EXPR_LOGICAL_NOT]      = "!",
	[EXPR_FUNC]             = "CALL "
};

void ir_print_sequence(struct ir_sequence *ir)
{
	struct ir_instruction *inst;

	VECTOR_ITER(&ir->seq, inst) {
		if (inst->tag == IR_TEST) {
			printf("test\t");
			ir_print_operand(&inst->lhs);
		} else if (inst->tag == IR_PUSH) {
			printf("push\t");
			ir_print_operand(&inst->lhs);
		} else if (inst->tag == IR_LOAD) {
			printf("t%d\t= ", inst->target);
			ir_print_operand(&inst->lhs);
		} else if (inst->tag == EXPR_ASSIGN) {
			if (inst->lhs.op_type == IR_OPERAND_TEMP_REG) {
				printf("M[");
				ir_print_operand(&inst->lhs);
				putchar(']');
			} else {
				ir_print_operand(&inst->lhs);
			}
			printf("\t= ");
			ir_print_operand(&inst->rhs);
		} else if (inst->tag == EXPR_DEREFERENCE) {
			printf("t%d\t= M[", inst->target);
			ir_print_operand(&inst->lhs);
			putchar(']');
		} else if (TAG_IS_UNARY(inst->tag) || inst->tag == EXPR_FUNC) {
			printf("t%d\t= %s", inst->target, expr_str[inst->tag]);
			ir_print_operand(&inst->lhs);
		} else {
			printf("t%d\t= ", inst->target);
			ir_print_operand(&inst->lhs);
			printf(" %s ", expr_str[inst->tag]);
			ir_print_operand(&inst->rhs);
		}
		putchar('\n');
	}
}
