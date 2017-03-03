/*
 * ir.h
 */

#ifndef FCC_IR_H
#define FCC_IR_H

#include "ast.h"
#include "vector.h"

#define IR_OPERAND_TERMINAL 0
#define IR_OPERAND_TEMP_REG 1

/*
 * An operand for a 3-point IR instruction.
 * It can either be an AST node representing a constant/ID,
 * or a temporary "register".
 */
struct ir_operand {
	int op_type;
	union {
		struct ast_node *term;
		int reg;
	};
};

struct ir_instruction {
	int tag;
	int target;
	unsigned int type_flags;
	struct ir_operand lhs;
	struct ir_operand rhs;
};

struct ir_sequence {
	struct vector seq;
};

void ir_init(struct ir_sequence *ir);
void ir_destroy(struct ir_sequence *ir);

void ir_parse(struct ir_sequence *ir, struct ast_node *expr);

void ir_print_sequence(struct ir_sequence *ir);

#endif /* FCC_IR_H */
