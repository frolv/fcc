/*
 * src/x86.h
 * Copyright (C) 2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FCC_X86_H
#define FCC_X86_H

#include <stdint.h>

#include "asg.h"
#include "local.h"
#include "vector.h"

enum {
	X86_MOV,
	X86_PUSH,
	X86_POP,
	X86_LEA,
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
	X86_JMP,
	X86_JE,
	X86_JG,
	X86_JGE,
	X86_JL,
	X86_JLE,
	X86_JNE,
	X86_JZ,
	X86_JNZ,
	X86_MOVZB,
	X86_CMP,
	X86_TEST,
	X86_CDQ,
	X86_RET,
	X86_CALL,
	X86_LABEL,
	X86_NAMED_LABEL
};

enum {
	X86_GPR_AX,
	X86_GPR_BX,
	X86_GPR_CX,
	X86_GPR_DX,
	X86_GPR_SI,
	X86_GPR_DI,
	X86_GPR_SP,
	X86_GPR_BP,
	X86_GPR_AL,
	X86_GPR_AH,
	X86_GPR_CH,
	X86_GPR_CL,
	X86_GPR_ANY
};

enum {
	X86_OPERAND_GPR,
	X86_OPERAND_CONSTANT,
	X86_OPERAND_UCONSTANT,
	X86_OPERAND_LABEL,
	X86_OPERAND_FUNC,
	X86_OPERAND_OFFSET
};

struct x86_operand {
	int type;
	union {
		int gpr;
		int constant;
		int label;
		char *func;
		struct {
			int16_t off;
			int16_t gpr;
		} offset;
	};
};

struct x86_instruction {
	uint16_t instruction;
	uint16_t size;
	union {
		const char *lname;
		int lnum;
	};
	struct x86_operand op1;
	struct x86_operand op2;
	struct x86_operand op3;
};

enum {
	X86_GPRVAL_NONE,
	X86_GPRVAL_NODE,
	X86_GPRVAL_TMPREG
};

struct x86_gprval {
	int tag;
	int used;
	union {
		int tmp_reg;
		struct ast_node *node;
	};
};

struct x86_sequence {
	struct vector seq;
	struct local_vars *locals;
	struct x86_gprval gprs[8];
	struct {
		int size;
		int *regs;
	} tmp_reg;
	int label;
};

void x86_seq_init(struct x86_sequence *seq, struct local_vars *locals);
void x86_seq_destroy(struct x86_sequence *seq);

void x86_begin_function(struct x86_sequence *seq, const char *fname);
void x86_end_function(struct x86_sequence *seq);
void x86_grow_stack(struct x86_sequence *seq, size_t bytes);
void x86_shrink_stack(struct x86_sequence *seq, size_t bytes);
void x86_translate(struct x86_sequence *seq, struct graph_node *g);

int x86_write_instruction(struct x86_instruction *inst, char *out);

#endif /* FCC_X86_H */
