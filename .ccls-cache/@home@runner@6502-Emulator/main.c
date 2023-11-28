#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define byte unsigned char
#define word unsigned short

#define HIGH 0x01
#define LOW  0x00

#define MEMORY_MAX 1024*64
#define MAX_REGISTERS 3

#define IN_MEMORY(expr)\
	(expr < MEMORY_MAX) ? HIGH : LOW;\

struct cpu_t {
	word SP;
	word PC;

	byte CF : HIGH;
	byte ZF : HIGH;
	byte IF : HIGH;
	byte DF : HIGH;
	byte OF : HIGH;
	byte NF : HIGH;

	byte *memory;
	byte *registers;
};

void reset_cpu(struct cpu_t* c) {
	c->PC = 0xFFFC;
	c->SP = 0x0100;

	c->CF = LOW;
	c->ZF = LOW;
	c->IF = LOW;
	c->DF = LOW;
	c->OF = LOW;
	c->NF = LOW;

	for (int i = 0; i < MEMORY_MAX; i++)	c->memory[i]	= 0x0000;
	for (int i = 0; i < MAX_REGISTERS; i++) c->registers[i] = 0x0000;
}


void init_cpu(struct cpu_t* c) {
	c->memory = (byte*)malloc(MEMORY_MAX * sizeof(byte));
	c->registers = (byte*)malloc(MAX_REGISTERS * sizeof(byte));
	reset_cpu(c);
}

void free_cpu(struct cpu_t* c) {
	free(c->memory);
	free(c->registers);
	free(c);
}

void cpu_ldw(struct cpu_t* c, word data, uint32_t addr) {
	c->memory[addr] = data & 0xFF;
	c->memory[addr + 1] = (data >> 8);
}


void cpu_lda_set_flags(struct cpu_t* c) {
	c->ZF = (c->registers[0x00] == 0x00);
	c->NF = (c->registers[0x00] & 0b10000000) > 0;
}

#define INST_HLT 0x00
#define INST_JSR 0x20

#define INST_LDA_IMM 0xA9
#define INST_LDA_ZEP 0xA5
#define INST_LDA_ZPX 0xB5
#define INST_LDA_ABS 0xAD
#define INST_LDA_ABX 0xBD
#define INST_LDA_ABY 0xB9

#define STDOUT 0x00
#define STDIN  0x01
#define STDERR 0x02

int cpu_run(struct cpu_t* c) {
	while (1) {
		byte opcode = c->memory[c->PC++];
		switch (opcode) {
		case 0x00: 
		{
			byte excode = c->memory[c->PC++];
			return (int)excode;
		}
		// LDA IMMEDIATE
		case 0xA9: 
		{
			byte val = c->memory[c->PC++];
			c->registers[0x00] = val;
			cpu_lda_set_flags(c);
		} break;

		// LDA ZERO PAGE 
		case 0xA5: 
		{ 
			byte addr = c->memory[c->PC++];
			c->registers[0x00] = c->memory[addr];
			cpu_lda_set_flags(c);
		} break;

		// LDA ZERO PAGE X 
		case 0xB5: 
		{
			byte addr = c->memory[c->PC++];
			addr += c->registers[0x01];
			if ((int)addr < MEMORY_MAX) {
				c->registers[0x00] = c->memory[addr];
				cpu_lda_set_flags(c);
			}
		} break;
		 
		// LDA ABSOLUTE
		case 0xAD: 
		{
			word addr = c->memory[c->PC++];
			addr |= (c->memory[c->PC++] << 8);

			c->registers[0x00] = c->memory[addr];

		} break;

		// LDA ABSOLUTE X
		case 0xBD: 
		{
			word addr = c->memory[c->PC++];
			addr |= (c->memory[c->PC++] << 8);

			c->registers[0x00] = c->memory[addr];
			c->registers[0x00] += c->registers[0x01];
		} break;

		// LDA ABSOLUTE Y
		case 0xB9: 
		{
			word addr = c->memory[c->PC++];
			addr |= (c->memory[c->PC++] << 8);

			c->registers[0x00] = c->memory[addr];
			c->registers[0x00] += c->registers[0x02];
		} break;

			
		// JSR
		case 0x20: 
		{
			word addr = c->memory[c->PC++];
			addr |= (c->memory[c->PC++] << 8);

			cpu_ldw(c, c->PC - 1, c->SP);
			c->memory[c->SP] = c->PC - 1;
			c->PC = addr;
		} break;

		// Invalid Operation
		default: return -1;
		}
	}
	return 0;
} 

byte cpu_load(struct cpu_t* c, byte* progam, size_t size) {
	for (int i = 0; i < size; i++) {
		c->memory[0xFFFC+i] = progam[i];
	}
	return 0;
}

int main(int argc, char** argv) {
	struct cpu_t* cpu = (struct cpu_t*)malloc(sizeof(struct cpu_t));
	init_cpu(cpu);
	reset_cpu(cpu);

	byte program[] = {
		0xA9, 0x05,
		0x00, 0x03,
	};
	
	cpu_load(cpu, program, sizeof(program));
	byte excode = cpu_run(cpu);
	if (excode != 0) {
		FILE* stream = (excode == 1)? stderr : stdout;
		fprintf(stream, "Exit Code (%d)\n", excode);
	};
	free_cpu(cpu);
	return (excode == (0 | 1))? excode : 0;
}