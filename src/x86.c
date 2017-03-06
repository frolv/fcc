/*
 * x86.c
 */

#include <stdlib.h>
#include <string.h>

#include "gen.h"
#include "ir.h"
#include "types.h"
#include "x86.h"

void x86_seq_init(struct x86_sequence *seq, struct local_vars *locals)
{
	vector_init(&seq->seq, sizeof (struct x86_instruction));
	seq->locals = locals;
	seq->tmp_reg.size = 0;
	seq->tmp_reg.regs = malloc(NUM_TEMP_REGS * sizeof *seq->tmp_reg.regs);

	/* -1 indicates not in use */
	memset(seq->tmp_reg.regs, 0xFF,
	       NUM_TEMP_REGS * sizeof *seq->tmp_reg.regs);
}

void x86_seq_destroy(struct x86_sequence *seq)
{
	vector_destroy(&seq->seq);
	free(seq->tmp_reg.regs);
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
                                         struct ir_instruction *i)
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

static void translate_arithmetic_instruction(struct x86_sequence *seq,
                                             struct ir_instruction *i)
{
	return __translate_generic(seq, i, x86_expr_instructions[i->tag], 1);
}

static void translate_rshift_instruction(struct x86_sequence *seq,
                                         struct ir_instruction *i)
{
	return __translate_generic(seq, i,
	                           i->type_flags & QUAL_UNSIGNED
	                           ? X86_SHR : X86_SAR, 1);

}

/*
 * translate_comparison_instruction:
 * Translate a comparison instruction with value into x86 instruction.
 */
static void translate_comparison_instruction(struct x86_sequence *seq,
                                             struct ir_instruction *i)
{
	struct x86_instruction out;

	__translate_generic(seq, i, X86_CMP, 0);

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
                                                 struct ir_instruction *i)
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
}

/*
 * translate_division_instruction:
 * Translate a div or mod IR instruction to x86.
 */
static void translate_division_instruction(struct x86_sequence *seq,
                                           struct ir_instruction *i)
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
}

/*
 * translate_unary_instruction:
 * Translate a unary arithmetic/logical IR instruction to x86.
 */
static void translate_unary_instruction(struct x86_sequence *seq,
                                        struct ir_instruction *i)
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

static void (*tr_func[])(struct x86_sequence *, struct ir_instruction *) = {
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
	[EXPR_FUNC] = NULL
};

static void x86_write_instruction(struct x86_instruction *inst, char *out);

void x86_seq_translate_ir(struct x86_sequence *seq, struct ir_sequence *ir)
{
	struct ir_instruction *i;

	VECTOR_ITER(&ir->seq, i) {
		if (tr_func[i->tag])
			tr_func[i->tag](seq, i);
	}

	struct x86_instruction *x;
	char buf[64];
	VECTOR_ITER(&seq->seq, x) {
		x86_write_instruction(x, buf);
		printf("\t%s\n", buf);
	}
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
	[X86_MOVZB]     = "movzb",
	[X86_CMP]       = "cmp",
	[X86_CDQ]       = "cdq"
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
	case X86_OPERAND_LABEL:
		n = 0;
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
static void x86_write_instruction(struct x86_instruction *inst, char *out)
{
	int operands;

	operands = x86_num_operands(inst->instruction);

	out += sprintf(out, "%s%s",
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
