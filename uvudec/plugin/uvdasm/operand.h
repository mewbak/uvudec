/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#ifndef UVDASM_OPERAND_H
#define UVDASM_OPERAND_H

#include "uvd/assembly/instruction.h"
#include "uvd/core/std_iterator.h"
#include "uvdasm/function.h"

class UVDDisasmOperandShared : public UVDOperandShared
{
public:
	UVDDisasmOperandShared();
	~UVDDisasmOperandShared();
	uv_err_t deinit();

	virtual uv_err_t parseOperand(UVDASInstructionIterator *uvdIter, UVDDisasmOperand **out);

	//Returns error if it isn't an immediate
	//uv_err_t getImmediateSize(uint32_t *immediateSizeOut);

	static uv_err_t uvd_parsed2opshared(const UVDConfigValue *parsed_type, UVDDisasmOperandShared **op_shared_in);

public:
	/*
	Register, memory, immediate
	UV_DISASM_DATA_*
	*/
	uint32_t m_type;

	/*
	Register: malloc'd register name
	Immediate: single integer (not a pointer) indicating size in bits
		Try to phase out immediate defines
	Memory:
	*/
	union
	{
		void *m_type_specific;
		int m_immediate_size;
		UVDDisasmFunctionShared *m_func;
		/* struct uvd_reg_shared_t *m_reg; */
	};

	/* Symbolic name as defined in the .op file's USAGE field */
	std::string m_name;
	
	//UVDOperandShared *m_next;
};

/*
Involves a numeric value of some kind
May be absolutly fixed as per the instruction, or a generic signed or unsigned constant of a specific size
*/
class UVDDisasmNumericOperandShared : public UVDDisasmOperandShared
{
public:
	UVDDisasmNumericOperandShared();
	~UVDDisasmNumericOperandShared();
};

/*
Only a specific value (or maybe range of values?) are possible
Ex: JMP 0x20 on Z80, INT3 on x86
These have the operand hard coded as part of the opcode meaning
and is different than say INT/s8 where there is an operand that encodes the number
This has similarities to an opcode, but one is is syntax, the other is in usage
*/
class UVDDisasmConstantOperandShared : public UVDDisasmNumericOperandShared
{
public:
	UVDDisasmConstantOperandShared();
	~UVDDisasmConstantOperandShared();
		
	virtual uv_err_t parseOperand(UVDASInstructionIterator *uvdIter, UVDDisasmOperand **out);

public:
	//A constant value associated with a mmemoric
	uint32_t m_value;
};

class UVDDisasmOperand : public UVDOperand
{
public:
	UVDDisasmOperand();
	~UVDDisasmOperand();
	virtual uv_err_t init();
	virtual uv_err_t deinit();

	//Convenience cast
	UVDDisasmOperandShared *getShared();

	//uv_err_t uvd_parsed2opshared(const struct uvd_parsed_t *parsed_type, UVDOperandShared **op_shared_in);
	//DEPRECATED: move things to shared parsing so we can alloc instead of using union stuff
	virtual uv_err_t parseOperand(UVDASInstructionIterator *uvdIter);

	virtual uv_err_t printDisassemblyOperand(std::string &out);
	//uv_err_t print_disasm_operand(char *buff, unsigned int buffsz, unsigned int *buff_used_in);

	//Get a variable mapping suitable for scripting
	//If name returns empty, is not applicable
	uv_err_t getVariable(std::string &name, UVDVarient &value);

	//Does not have to be original size
	//Must match signedness
	uv_err_t getUI32Representation(uint32_t &i);
	uv_err_t getI32Representation(int32_t &i);

	//Created to give representations that might allow for binary to pratical value translations
	//Ex: relative jump contains binary 3, but really is PC + 3 = say 123
	uv_err_t getUI32RepresentationAdjusted(uint32_t &i);
	uv_err_t getI32RepresentationAdjusted(int32_t &i);

	//uv_err_t setInstruction(UVDInstruction *instruction);
	
private:
	/* 
	Additional information for this operand, such as a value larger than simple field supports 
	Interpretation is operand specific and determined by m_shared
	
	REG: std::string  to register name
	IMMEDIATE: larger than sizeof(value): pointer to raw byte stream
		Support for such data isn't planned in near future
	*/
	union
	{
		void *m_extra;
		
		UVDDisasmFunction *m_func;

		/* Many operands are immediates of some time or another */
		uint8_t m_ui8;
		int8_t m_i8;

		uint16_t m_ui16;
		int16_t m_i16;

		uint32_t m_ui32;
		int32_t m_i32;
	};
};

class UVDDisasmConstantOperand : public UVDDisasmOperand
{
public:
	UVDDisasmConstantOperand();
	~UVDDisasmConstantOperand();

	UVDDisasmConstantOperandShared *getShared();

	virtual uv_err_t printDisassemblyOperand(std::string &out);
};

#endif

