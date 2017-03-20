/*
 * ir.h
 */

#ifndef FCC_IR_H
#define FCC_IR_H

#include <stdint.h>

#include "ast.h"
#include "asg.h"
#include "vector.h"

#define NUM_TEMP_REGS 31

#define IR_OPERAND_AST_NODE 0
#define IR_OPERAND_TEMP_REG 1
#define IR_OPERAND_NODE_OFF 2
#define IR_OPERAND_REG_OFF  3

/*
 * An operand for a 3-point IR instruction.
 * It can either be an AST node representing a constant/ID,
 * a temporary "register", or an offset from either.
 */
struct ir_operand {
	int op_type;
	union {
		struct ast_node *node;
		int reg;
	};
	size_t off;
};

#define IR_TEST   0xA0
#define IR_PUSH   0xA1
#define IR_LOAD   0xA2

struct ir_instruction {
	uint16_t tag;
	int16_t target;
	struct type_information type;
	struct ir_operand lhs;
	struct ir_operand rhs;
};

struct ir_sequence {
	struct vector seq;
};

void ir_init(struct ir_sequence *ir);
void ir_destroy(struct ir_sequence *ir);
void ir_clear(struct ir_sequence *ir);
void ir_parse_expr(struct ir_sequence *ir, struct ast_node *expr, int cond);

void ir_print_sequence(struct ir_sequence *ir);

#endif /* FCC_IR_H */
