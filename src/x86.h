/*
 * x86.h
 */

#ifndef FCC_X86_H
#define FCC_X86_H

#include <stdint.h>

#include "local.h"
#include "vector.h"

enum {
	X86_MOV,
	X86_PUSH,
	X86_POP,
	X86_ADD,
	X86_SUB,
	X86_OR,
	X86_XOR,
	X86_AND,
	X86_SHL,
	X86_SHR,
	X86_SAR,
	X86_IMUL,
	X86_DIV,
	X86_NOT,
	X86_NEG,
	X86_SETE,
	X86_SETG,
	X86_SETGE,
	X86_SETL,
	X86_SETLE,
	X86_SETNE,
	X86_MOVZB,
	X86_CMP,
	X86_CDQ
};

enum {
	X86_GPR_ANY,
	X86_GPR_AL,
	X86_GPR_AH,
	X86_GPR_AX,
	X86_GPR_BX,
	X86_GPR_CX,
	X86_GPR_DX,
	X86_GPR_SI,
	X86_GPR_DI,
	X86_GPR_SP,
	X86_GPR_BP
};

enum {
	X86_OPERAND_GPR,
	X86_OPERAND_CONSTANT,
	X86_OPERAND_LABEL,
	X86_OPERAND_OFFSET
};

struct x86_operand {
	int type;
	union {
		int gpr;
		int constant;
		char *label;
		struct {
			int off;
			int gpr;
		} offset;
	};
};

struct x86_instruction {
	uint16_t instruction;
	uint16_t size;
	struct x86_operand op1;
	struct x86_operand op2;
	struct x86_operand op3;
};

struct x86_sequence {
	struct vector seq;
	struct local_vars *locals;
	struct {
		int size;
		int *regs;
	} tmp_reg;
};

void x86_seq_init(struct x86_sequence *seq, struct local_vars *locals);
void x86_seq_destroy(struct x86_sequence *seq);
void x86_seq_translate_ir(struct x86_sequence *seq, struct ir_sequence *ir);

#endif /* FCC_X86_H */
