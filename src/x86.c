/*
 * x86.c
 */

#include <stdlib.h>
#include <string.h>

#include "gen.h"
#include "ir.h"
#include "types.h"
#include "x86.h"

static int curr_label = 0;

void x86_seq_init(struct x86_sequence *seq, struct local_vars *locals)
{
	vector_init(&seq->seq, sizeof (struct x86_instruction));
	seq->locals = locals;
	seq->tmp_reg.size = 0;
	seq->tmp_reg.regs = malloc(NUM_TEMP_REGS * sizeof *seq->tmp_reg.regs);

	/* -1 indicates not in use */
	memset(seq->tmp_reg.regs, 0xFF,
	       NUM_TEMP_REGS * sizeof *seq->tmp_reg.regs);
	seq->label = curr_label;
}

void x86_seq_destroy(struct x86_sequence *seq)
{
	vector_destroy(&seq->seq);
	free(seq->tmp_reg.regs);
	curr_label = seq->label;
}

/*
 * tmp_reg_push:
 * Create an instruction to push register `gpr` into temporary register
 * `tmp_reg`, and update temporary register offsets.
 */
static void tmp_reg_push(struct x86_sequence *seq, int tmp_reg, int gpr)
{
	struct x86_instruction out;
	int i;

	for (i = 0; i < NUM_TEMP_REGS; ++i) {
		if (seq->tmp_reg.regs[i] != -1)
			seq->tmp_reg.regs[i] += 4;
	}
	seq->tmp_reg.regs[tmp_reg] = 0;

	if (gpr != -1) {
		out.instruction = X86_PUSH;
		out.size = 0;
		out.op1.type = X86_OPERAND_GPR;
		out.op1.gpr = gpr;
		vector_append(&seq->seq, &out);
	}
}

/*
 * tmp_reg_push:
 * Create an instruction to pop temporary register `tmp_reg` into register
 * `gpr`, and update temporary register offsets.
 */
static void tmp_reg_pop(struct x86_sequence *seq, int tmp_reg, int gpr)
{
	struct x86_instruction out;
	int i;

	for (i = 0; i < NUM_TEMP_REGS; ++i) {
		if (seq->tmp_reg.regs[i] != -1)
			seq->tmp_reg.regs[i] -= 4;
	}
	seq->tmp_reg.regs[tmp_reg] = -1;

	if (gpr != -1) {
		out.instruction = X86_POP;
		out.size = 0;
		out.op1.type = X86_OPERAND_GPR;
		out.op1.gpr = gpr;
		vector_append(&seq->seq, &out);
	}
}

/*
 * ir_to_x86_operand:
 * Converts an operand from IR to x86.
 */
static void ir_to_x86_operand(struct x86_sequence *seq,
                              struct ir_operand *i,
                              struct x86_operand *x)
{
	struct local *l;

	if (i->op_type == IR_OPERAND_TERMINAL) {
		switch (i->term->tag) {
		case NODE_IDENTIFIER:
			/* local variable: offset from base pointer */
			l = local_find(seq->locals, i->term->lexeme);
			x->type = X86_OPERAND_OFFSET;
			x->offset.off = -(l->offset);
			x->offset.gpr = X86_GPR_BP;
			break;
		case NODE_CONSTANT:
			if (i->term->expr_flags & QUAL_UNSIGNED ||
			    FLAGS_IS_PTR(i->term->expr_flags))
				x->type = X86_OPERAND_UCONSTANT;
			else
				x->type = X86_OPERAND_CONSTANT;
			x->constant = i->term->value;
			break;
		case NODE_STRLIT:
			x->type = X86_OPERAND_LABEL;
			break;
		}
	} else {
		x->type = X86_OPERAND_OFFSET;
		x->offset.off = seq->tmp_reg.regs[i->reg];
		x->offset.gpr = X86_GPR_SP;
	}
}

/*
 * x86_load_value:
 * Create an x86 instruction to load variable `var` into GPR `gpr`.
 */
static int x86_load_value(struct x86_sequence *seq,
                          struct ir_operand *val,
                          int gpr)
{
	struct x86_instruction out;

	if (gpr == X86_GPR_ANY)
		gpr = X86_GPR_AX;

	out.instruction = X86_MOV;
	out.size = type_size(val->term->expr_flags);
	ir_to_x86_operand(seq, val, &out.op1);
	out.op2.type = X86_OPERAND_GPR;
	out.op2.gpr = gpr;

	vector_append(&seq->seq, &out);
	return gpr;
}

/*
 * x86_load_tmp_reg:
 * Create an x86 instruction to load temporary register `tmp_reg`
 * into GPR `gpr` or X86_GPR_ANY.
 * If `tmp_reg` is at the top of the stack, pop it.
 * Return the GPR into which `tmp_reg` was placed.
 */
static int x86_load_tmp_reg(struct x86_sequence *seq,
                            struct ir_operand *tmp_reg,
                            int gpr)
{
	struct x86_instruction out, last;
	int poparg;

	poparg = gpr;
	ir_to_x86_operand(seq, tmp_reg, &out.op1);

	if (!out.op1.offset.off) {
		/*
		 * Item is on top of the stack.
		 * Check to see if the most recent instruction was a push
		 * of the same register. If so, delete it as it is unnecessary.
		 */
		vector_get(&seq->seq, seq->seq.nmembs - 1, &last);
		if (last.instruction == X86_PUSH) {
			vector_pop(&seq->seq, NULL);
			poparg = -1;

			/* Keep item in the register it was popped from. */
			if (gpr == X86_GPR_ANY) {
				tmp_reg_pop(seq, tmp_reg->reg, poparg);
				return last.op1.gpr;
			}

			if (last.op1.gpr != gpr) {
				/* Move item from last register to target. */
				out.instruction = X86_MOV;
				out.size = 4;
				out.op1.type = X86_OPERAND_GPR;
				out.op1.gpr = last.op1.gpr;
				out.op2.type = X86_OPERAND_GPR;
				out.op2.gpr = gpr;
				vector_append(&seq->seq, &out);
			}
		}
		tmp_reg_pop(seq, tmp_reg->reg, poparg);
	} else {
		if (gpr == X86_GPR_ANY)
			gpr = X86_GPR_AX;

		out.instruction = X86_MOV;
		out.size = 4;
		out.op2.type = X86_OPERAND_GPR;
		out.op2.gpr = gpr;
		vector_append(&seq->seq, &out);
	}
	return gpr;
}

/*
 * translate_assign_instruction:
 * Converts an IR assignment instruction to a series of x86 instructions.
 */
static void translate_assign_instruction(struct x86_sequence *seq,
                                         struct ir_instruction *i,
                                         int cond)
{
	struct x86_instruction out;
	int gpr;

	if (i->lhs.op_type == IR_OPERAND_TERMINAL) {
		out.instruction = X86_MOV;
		out.size = type_size(i->type_flags);
		ir_to_x86_operand(seq, &i->lhs, &out.op2);

		if (i->rhs.op_type == IR_OPERAND_TERMINAL) {
			switch (i->rhs.term->tag) {
			case NODE_IDENTIFIER:
				x86_load_value(seq, &i->rhs, X86_GPR_AX);
				out.op1.type = X86_OPERAND_GPR;
				out.op1.gpr = X86_GPR_AX;
				break;
			case NODE_CONSTANT:
				ir_to_x86_operand(seq, &i->rhs, &out.op1);
				break;
			case NODE_STRLIT:
				break;
			}
		} else {
			gpr = x86_load_tmp_reg(seq, &i->rhs, X86_GPR_ANY);
			out.op1.type = X86_OPERAND_GPR;
			out.op1.gpr = gpr;
		}
	} else {
		/* TODO: this */
	}

	vector_append(&seq->seq, &out);

	(void)cond;
}

/*
 * __translate_generic:
 * Translate generic x86 instruction `instruction` from IR.
 */
static void __translate_generic(struct x86_sequence *seq,
                                   struct ir_instruction *i,
                                   int instruction, int push)
{
	struct x86_instruction out;
	struct x86_operand *op;
	int ax, reg;

	out.instruction = instruction;
	out.size = type_size(i->type_flags);
	ax = 0;

	if (i->lhs.op_type == IR_OPERAND_TERMINAL) {
		switch (i->lhs.term->tag) {
		case NODE_IDENTIFIER:
			x86_load_value(seq, &i->lhs, X86_GPR_AX);
			out.op2.type = X86_OPERAND_GPR;
			out.op2.gpr = X86_GPR_AX;
			ax = 1;
			break;
		case NODE_CONSTANT:
			ir_to_x86_operand(seq, &i->lhs, &out.op1);
			break;
		}
	} else {
		x86_load_tmp_reg(seq, &i->lhs, X86_GPR_AX);
		out.op2.type = X86_OPERAND_GPR;
		out.op2.gpr = X86_GPR_AX;
		ax = 1;
	}

	/* Output register AX has been set. */
	if (ax) {
		reg = X86_GPR_DX;
		op = &out.op1;
	} else {
		reg = X86_GPR_AX;
		op = &out.op2;
	}

	if (i->rhs.op_type == IR_OPERAND_TERMINAL) {
		switch (i->rhs.term->tag) {
		case NODE_IDENTIFIER:
			x86_load_value(seq, &i->rhs, reg);
			op->type = X86_OPERAND_GPR;
			op->gpr = reg;
			break;
		case NODE_CONSTANT:
			ir_to_x86_operand(seq, &i->rhs, &out.op1);
			break;
		}
	} else {
		x86_load_tmp_reg(seq, &i->rhs, reg);
		op->type = X86_OPERAND_GPR;
		op->gpr = reg;
	}

	vector_append(&seq->seq, &out);
	if (push)
		tmp_reg_push(seq, i->target, X86_GPR_AX);
}

static int x86_expr_instructions[] = {
	[EXPR_OR]       = X86_OR,
	[EXPR_XOR]      = X86_XOR,
	[EXPR_AND]      = X86_AND,
	[EXPR_LSHIFT]   = X86_SHL,
	[EXPR_ADD]      = X86_ADD,
	[EXPR_SUB]      = X86_SUB,
	[EXPR_EQ]       = X86_SETE,
	[EXPR_NE]       = X86_SETNE,
	[EXPR_LT]       = X86_SETL,
	[EXPR_GT]       = X86_SETG,
	[EXPR_LE]       = X86_SETLE,
	[EXPR_GE]       = X86_SETGE
};

static int x86_jump_instructions[] = {
	[EXPR_LOGICAL_NOT]      = X86_JNE,
	[EXPR_EQ]               = X86_JNE,
	[EXPR_NE]               = X86_JE,
	[EXPR_LT]               = X86_JGE,
	[EXPR_GT]               = X86_JLE,
	[EXPR_LE]               = X86_JG,
	[EXPR_GE]               = X86_JL
};

static void translate_arithmetic_instruction(struct x86_sequence *seq,
                                             struct ir_instruction *i,
                                             int cond)
{
	return __translate_generic(seq, i, x86_expr_instructions[i->tag], 1);

	(void)cond;
}

static void translate_rshift_instruction(struct x86_sequence *seq,
                                         struct ir_instruction *i,
                                         int cond)
{
	return __translate_generic(seq, i,
	                           i->type_flags & QUAL_UNSIGNED
	                           ? X86_SHR : X86_SAR, 1);

	(void)cond;

}

/*
 * translate_comparison_instruction:
 * Translate a comparison instruction with value into x86 instruction.
 */
static void translate_comparison_instruction(struct x86_sequence *seq,
                                             struct ir_instruction *i,
                                             int cond)
{
	struct x86_instruction out;

	__translate_generic(seq, i, X86_CMP, 0);

	if (cond)
		return;

	out.instruction = x86_expr_instructions[i->tag];
	out.size = 0;
	out.op1.type = X86_OPERAND_GPR;
	out.op1.gpr = X86_GPR_AL;

	vector_append(&seq->seq, &out);

	out.instruction = X86_MOVZB;
	out.op2.type = X86_OPERAND_GPR;
	out.op2.gpr = X86_GPR_AX;

	vector_append(&seq->seq, &out);
	tmp_reg_push(seq, i->target, X86_GPR_AX);
}

/*
 * translate_multiplicative_instruction:
 * Translate a multiplication from IR to x86.
 */
static void translate_multiplicative_instruction(struct x86_sequence *seq,
                                                 struct ir_instruction *i,
                                                 int cond)
{
	struct x86_instruction out;
	struct x86_operand *op;
	int ax, reg;

	out.instruction = X86_IMUL;
	out.size = 0;
	out.op3.type = X86_OPERAND_GPR;
	out.op3.gpr = X86_GPR_AX;
	ax = 0;

	if (i->lhs.op_type == IR_OPERAND_TERMINAL) {
		switch (i->lhs.term->tag) {
		case NODE_IDENTIFIER:
			x86_load_value(seq, &i->lhs, X86_GPR_AX);
			out.op2.type = X86_OPERAND_GPR;
			out.op2.gpr = X86_GPR_AX;
			ax = 1;
			break;
		case NODE_CONSTANT:
			ir_to_x86_operand(seq, &i->lhs, &out.op1);
			break;
		}
	} else {
		x86_load_tmp_reg(seq, &i->lhs, X86_GPR_AX);
		out.op2.type = X86_OPERAND_GPR;
		out.op2.gpr = X86_GPR_AX;
		ax = 1;
	}

	if (ax) {
		reg = X86_GPR_DX;
		op = &out.op1;
	} else {
		reg = X86_GPR_AX;
		op = &out.op2;
	}

	if (i->rhs.op_type == IR_OPERAND_TERMINAL) {
		switch (i->rhs.term->tag) {
		case NODE_IDENTIFIER:
			x86_load_value(seq, &i->rhs, reg);
			op->type = X86_OPERAND_GPR;
			op->gpr = reg;
			break;
		case NODE_CONSTANT:
			ir_to_x86_operand(seq, &i->rhs, &out.op1);
			break;
		}
	} else {
		x86_load_tmp_reg(seq, &i->rhs, reg);
		op->type = X86_OPERAND_GPR;
		op->gpr = reg;
	}

	vector_append(&seq->seq, &out);
	tmp_reg_push(seq, i->target, X86_GPR_AX);

	(void)cond;
}

/*
 * translate_division_instruction:
 * Translate a div or mod IR instruction to x86.
 */
static void translate_division_instruction(struct x86_sequence *seq,
                                           struct ir_instruction *i,
                                           int cond)
{
	struct x86_instruction out;

	if (i->lhs.op_type == IR_OPERAND_TERMINAL)
		x86_load_value(seq, &i->lhs, X86_GPR_AX);
	else
		x86_load_tmp_reg(seq, &i->lhs, X86_GPR_AX);

	out.instruction = X86_CDQ;
	out.size = 0;
	vector_append(&seq->seq, &out);

	if (i->rhs.op_type == IR_OPERAND_TERMINAL)
		x86_load_value(seq, &i->rhs, X86_GPR_CX);
	else
		x86_load_tmp_reg(seq, &i->rhs, X86_GPR_CX);

	out.instruction = X86_DIV;
	out.size = 0;
	out.op1.type = X86_OPERAND_GPR;
	out.op1.gpr = X86_GPR_CX;

	vector_append(&seq->seq, &out);
	tmp_reg_push(seq, i->target,
	             i->tag == EXPR_DIV ? X86_GPR_AX : X86_GPR_DX);

	(void)cond;
}

/*
 * translate_unary_instruction:
 * Translate a unary arithmetic/logical IR instruction to x86.
 */
static void translate_unary_instruction(struct x86_sequence *seq,
                                        struct ir_instruction *i,
                                        int cond)
{
	struct x86_instruction out;
	int gpr;

	if (i->tag == EXPR_LOGICAL_NOT) {
		gpr = 0;
		out.instruction = X86_CMP;
		out.size = 0;

		if (i->lhs.op_type == IR_OPERAND_TERMINAL)
			gpr = x86_load_value(seq, &i->lhs, X86_GPR_ANY);
		else
			gpr = x86_load_tmp_reg(seq, &i->lhs, X86_GPR_ANY);

		out.op1.type = X86_OPERAND_CONSTANT;
		out.op1.constant = 0;
		out.op2.type = X86_OPERAND_GPR;
		out.op2.constant = gpr;
		vector_append(&seq->seq, &out);

		if (cond)
			return;

		out.instruction = X86_SETNE;
		out.op1.type = X86_OPERAND_GPR;
		out.op1.gpr = X86_GPR_AL;
		vector_append(&seq->seq, &out);

		out.instruction = X86_MOVZB;
		out.op2.type = X86_OPERAND_GPR;
		out.op2.gpr = X86_GPR_AX;
	} else {
		if (i->lhs.op_type == IR_OPERAND_TERMINAL)
			gpr = x86_load_value(seq, &i->lhs, X86_GPR_ANY);
		else
			gpr = x86_load_tmp_reg(seq, &i->lhs, X86_GPR_ANY);

		out.instruction = i->tag == EXPR_NOT ? X86_NOT : X86_NEG;
		out.size = 0;
		out.op1.type = X86_OPERAND_GPR;
		out.op1.gpr = gpr;
	}

	vector_append(&seq->seq, &out);
	tmp_reg_push(seq, i->target, gpr);
}

static void translate_test_instruction(struct x86_sequence *seq,
                                       struct ir_instruction *i,
                                       int cond)
{
	struct x86_instruction out;
	int gpr;

	if (i->lhs.op_type == IR_OPERAND_TERMINAL)
		gpr = x86_load_value(seq, &i->lhs, X86_GPR_ANY);
	else
		gpr = x86_load_tmp_reg(seq, &i->lhs, X86_GPR_ANY);

	out.instruction = X86_TEST;
	out.size = 0;
	out.op1.type = X86_OPERAND_GPR;
	out.op1.constant = gpr;
	out.op2.type = X86_OPERAND_GPR;
	out.op2.gpr = gpr;

	vector_append(&seq->seq, &out);
	(void)cond;
}

static void (*tr_func[])(struct x86_sequence *, struct ir_instruction *, int) = {
	[EXPR_ASSIGN] = translate_assign_instruction,
	[EXPR_LOGICAL_OR] = NULL,
	[EXPR_LOGICAL_AND] = NULL,
	[EXPR_OR] = translate_arithmetic_instruction,
	[EXPR_XOR] = translate_arithmetic_instruction,
	[EXPR_AND] = translate_arithmetic_instruction,
	[EXPR_EQ] = translate_comparison_instruction,
	[EXPR_NE] = translate_comparison_instruction,
	[EXPR_LT] = translate_comparison_instruction,
	[EXPR_GT] = translate_comparison_instruction,
	[EXPR_LE] = translate_comparison_instruction,
	[EXPR_GE] = translate_comparison_instruction,
	[EXPR_LSHIFT] = translate_arithmetic_instruction,
	[EXPR_RSHIFT] = translate_rshift_instruction,
	[EXPR_ADD] = translate_arithmetic_instruction,
	[EXPR_SUB] = translate_arithmetic_instruction,
	[EXPR_MULT] = translate_multiplicative_instruction,
	[EXPR_DIV] = translate_division_instruction,
	[EXPR_MOD] = translate_division_instruction,
	[EXPR_ADDRESS] = NULL,
	[EXPR_DEREFERENCE] = NULL,
	[EXPR_UNARY_MINUS] = translate_unary_instruction,
	[EXPR_NOT] = translate_unary_instruction,
	[EXPR_LOGICAL_NOT] = translate_unary_instruction,
	[EXPR_FUNC] = NULL,
	[IR_TEST] = translate_test_instruction
};

/*
 * x86_add_jump:
 * Create a jump instruction of type `type` to jump to label ID `label`.
 */
static void x86_add_jump(struct x86_sequence *seq, int type, int label)
{
	struct x86_instruction out;

	out.instruction = type;
	out.size = 0;
	out.op1.type = X86_OPERAND_LABEL;
	out.op1.label = label;

	vector_append(&seq->seq, &out);
}

/*
 * x86_add_label:
 * Create an instruction represting a label with ID `label`.
 */
static void x86_add_label(struct x86_sequence *seq, int label)
{
	struct x86_instruction out;

	out.instruction = X86_LABEL;
	out.size = label;
	vector_append(&seq->seq, &out);
}

static void x86_translate_expr(struct x86_sequence *seq,
                               struct ir_sequence *ir,
                               int cond)
{
	struct ir_instruction *i;

	VECTOR_ITER(&ir->seq, i) {
		if (tr_func[i->tag])
			tr_func[i->tag](seq, i, cond);
	}
}

void x86_translate_cond(struct x86_sequence *seq,
                        struct ir_sequence *ir,
                        struct asg_node_conditional *cond)
{
	struct ir_instruction i;
	int jtype, jfail, jend;

	jfail = seq->label++;
	jend = cond->fail ? seq->label++ : -1;

	ir_parse_expr(ir, cond->cond, 1);
	x86_translate_expr(seq, ir, 1);

	vector_pop(&ir->seq, &i);
	jtype = i.tag == IR_TEST ? X86_JZ : x86_jump_instructions[i.tag];

	x86_add_jump(seq, jtype, jfail);
	x86_translate(seq, cond->succ);

	if (cond->fail) {
		x86_add_jump(seq, X86_JMP, jend);
		x86_add_label(seq, jfail);
		x86_translate(seq, cond->fail);
	}

	x86_add_label(seq, cond->fail ? jend : jfail);
}

void x86_translate_for(struct x86_sequence *seq,
                       struct ir_sequence *ir,
                       struct asg_node_for *f)
{
	struct ir_instruction i;
	int jexit, jtest, jtype;

	jtest = seq->label++;
	jexit = seq->label++;

	ir_parse_expr(ir, f->init, 0);
	x86_translate_expr(seq, ir, 0);
	ir_clear(ir);

	x86_add_label(seq, jtest);
	ir_parse_expr(ir, f->cond, 1);
	x86_translate_expr(seq, ir, 1);

	vector_pop(&ir->seq, &i);
	jtype = i.tag == IR_TEST ? X86_JZ : x86_jump_instructions[i.tag];

	x86_add_jump(seq, jtype, jexit);
	ir_clear(ir);

	x86_translate(seq, f->body);

	ir_parse_expr(ir, f->post, 0);
	x86_translate_expr(seq, ir, 0);
	x86_add_jump(seq, X86_JMP, jtest);

	x86_add_label(seq, jexit);
}

void x86_translate_ret(struct x86_sequence *seq,
                       struct ir_sequence *ir,
                       struct asg_node_return *ret)
{
	struct x86_instruction out;
	struct ir_instruction i;
	struct ir_operand op;

	if (ret->retval) {
		if (ret->retval->tag <= NODE_STRLIT) {
			/* retval is a terminal, not an expression */
			op.op_type = IR_OPERAND_TERMINAL;
			op.term = ret->retval;
			x86_load_value(seq, &op, X86_GPR_AX);
		} else {
			ir_parse_expr(ir, ret->retval, 0);
			x86_translate_expr(seq, ir, 0);

			vector_pop(&ir->seq, &i);
			op.op_type = IR_OPERAND_TEMP_REG;
			op.reg = i.target;
			x86_load_tmp_reg(seq, &op, X86_GPR_AX);
		}
	}

	out.instruction = X86_RET;
	out.size = 0;
	vector_append(&seq->seq, &out);
}

/*
 * x86_translate:
 * Translate abstract semantic graph `g` into a sequence of x86 instruction.
 */
void x86_translate(struct x86_sequence *seq, struct graph_node *g)
{
	struct ir_sequence ir;

	ir_init(&ir);

	for (; g; g = g->next) {
		ir_clear(&ir);

		switch (g->type) {
		case ASG_NODE_STATEMENT:
			ir_parse_expr(&ir, ((struct asg_node_statement *)g)->ast,
			              0);
			x86_translate_expr(seq, &ir, 0);
			break;
		case ASG_NODE_CONDITIONAL:
			x86_translate_cond(seq, &ir,
			                   (struct asg_node_conditional *)g);
			break;
		case ASG_NODE_FOR:
			x86_translate_for(seq, &ir, (struct asg_node_for *)g);
			break;
		case ASG_NODE_RETURN:
			x86_translate_ret(seq, &ir, (struct asg_node_return *)g);
			break;
		}
	}

	ir_destroy(&ir);
}

static char *x86_instructions[] = {
	[X86_MOV]       = "mov",
	[X86_PUSH]      = "push",
	[X86_POP]       = "pop",
	[X86_ADD]       = "add",
	[X86_SUB]       = "sub",
	[X86_OR]        = "or",
	[X86_XOR]       = "xor",
	[X86_AND]       = "and",
	[X86_SHL]       = "shl",
	[X86_SHR]       = "shr",
	[X86_SAR]       = "sar",
	[X86_IMUL]      = "imul",
	[X86_DIV]       = "div",
	[X86_NOT]       = "not",
	[X86_NEG]       = "neg",
	[X86_SETE]      = "sete",
	[X86_SETG]      = "setg",
	[X86_SETGE]     = "setge",
	[X86_SETL]      = "setl",
	[X86_SETLE]     = "setle",
	[X86_SETNE]     = "setne",
	[X86_JMP]       = "jmp",
	[X86_JE]        = "je",
	[X86_JG]        = "jg",
	[X86_JGE]       = "jge",
	[X86_JL]        = "jl",
	[X86_JLE]       = "jle",
	[X86_JNE]       = "jne",
	[X86_JZ]        = "jz",
	[X86_MOVZB]     = "movzb",
	[X86_CMP]       = "cmp",
	[X86_TEST]      = "test",
	[X86_CDQ]       = "cdq",
	[X86_RET]       = "ret"
};

static char *x86_size_suffix[] = {
	[0]     = "",
	[1]     = "b",
	[2]     = "w",
	[4]     = "l"
};

static char *x86_gprs[] = {
	[X86_GPR_AL] = "al",
	[X86_GPR_AH] = "ah",
	[X86_GPR_AX] = "eax",
	[X86_GPR_BX] = "ebx",
	[X86_GPR_CX] = "ecx",
	[X86_GPR_DX] = "edx",
	[X86_GPR_SI] = "esi",
	[X86_GPR_DI] = "edi",
	[X86_GPR_SP] = "esp",
	[X86_GPR_BP] = "ebp"
};

static int x86_num_operands(int instruction)
{
	switch (instruction) {
	case X86_PUSH:
	case X86_POP:
	case X86_DIV:
	case X86_NOT:
	case X86_NEG:
	case X86_SETE:
	case X86_SETG:
	case X86_SETGE:
	case X86_SETL:
	case X86_SETLE:
	case X86_SETNE:
	case X86_JMP:
	case X86_JE:
	case X86_JG:
	case X86_JGE:
	case X86_JL:
	case X86_JLE:
	case X86_JNE:
	case X86_JZ:
		return 1;
	case X86_MOV:
	case X86_ADD:
	case X86_SUB:
	case X86_OR:
	case X86_XOR:
	case X86_AND:
	case X86_SHL:
	case X86_SHR:
	case X86_SAR:
	case X86_MOVZB:
	case X86_CMP:
	case X86_TEST:
		return 2;
	case X86_IMUL:
		return 3;
	default:
		return 0;
	};
}

static int x86_write_operand(struct x86_operand *op, char *out)
{
	int n;

	switch (op->type) {
	case X86_OPERAND_GPR:
		n = sprintf(out, "%%%s", x86_gprs[op->gpr]);
		break;
	case X86_OPERAND_CONSTANT:
		n = sprintf(out, "$%d", op->constant);
		break;
	case X86_OPERAND_UCONSTANT:
		n = sprintf(out, "$%u", op->constant);
		break;
	case X86_OPERAND_LABEL:
		n = sprintf(out, ".L%d", op->label);
		break;
	case X86_OPERAND_OFFSET:
		n = sprintf(out, "%d(%%%s)",
		            op->offset.off,
		            x86_gprs[op->offset.gpr]);
		break;
	default:
		n = 0;
		break;
	}

	return n;
}

/*
 * x86_write_instruction:
 * Write a single x86 instruction to buffer `out`.
 * `out` is assumed to be at least 64 bytes long.
 */
void x86_write_instruction(struct x86_instruction *inst, char *out)
{
	int operands;

	if (inst->instruction == X86_LABEL) {
		sprintf(out, ".L%d:", inst->size);
		return;
	}

	operands = x86_num_operands(inst->instruction);

	out += sprintf(out, "\t%s%s",
	               x86_instructions[inst->instruction],
	               x86_size_suffix[inst->size]);

	if (operands >= 1) {
		*out++ = ' ';
		out += x86_write_operand(&inst->op1, out);
		if (operands >= 2) {
			out += sprintf(out, ", ");
			out += x86_write_operand(&inst->op2, out);
			if (operands == 3) {
				out += sprintf(out, ", ");
				x86_write_operand(&inst->op3, out);
			}
		}
	}
}
