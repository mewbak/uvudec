/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#include "uvd/config.h"
#include <algorithm>
#include <ctype.h>
#include <errno.h>
//What we need...IS SOME ROPE!
#if USING_ROPE
#include <ext/rope>
#endif //USING_ROPE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include "uvd/architecture/architecture.h"
#include "uvd/util/debug.h"
#include "uvd/util/error.h"
#include "uvd/util/util.h"
#include "uvd/core/uvd.h"
#include "uvd/assembly/address.h"
#include "uvd/core/analysis.h"
#include "uvd/util/benchmark.h"
#include "uvd/compiler/assembly.h"
#include "uvd/data/data.h"
#include "uvd/language/format.h"
#include "uvd/assembly/instruction.h"
#include "uvd/language/language.h"
#include "uvd/util/types.h"
#include "uvd/core/runtime.h"

UVD *g_uvd = NULL;

/*
TODO: there was a paradigm shift between how to analyze functions
Moving towards DB based storage
Each of these accomplish essentially same thing, but different way
*/

#if 0
//FIXME: this code is nearly dead and should probably be removed
uv_err_t UVD::analyzeNewFunction(const UVDAnalyzedMemoryRange *memLoc, UVDAnalyzedFunction &analyzedFunction)
{
	uv_err_t rc = UV_ERR_GENERAL;
	UVDAnalyzedCode *analyzedCode = NULL;
	UVDDataChunk *dataChunk = NULL;
	
	uv_assert(memLoc);
	
	//analyzedFunction = new UVDAnalyzedFunction();
	
	analyzedCode = new UVDAnalyzedCode();
	analyzedFunction.m_code = analyzedCode;
	
	dataChunk = new UVDDataChunk();
	if( UV_FAILED(dataChunk->init(m_data, memLoc->m_min_addr, memLoc->m_max_addr)) )
	{
		UV_DEBUG(rc);
		goto error;
	}
	analyzedCode->m_dataChunk = dataChunk;
	
	//Ready, perform analysis
	if( UV_FAILED(analyzeCode(*analyzedCode)) )
	{
		UV_DEBUG(rc);
		goto error;
	}
	
	/*
	Block analysis is not yet done yet, which would make this hard/duplicate work
	//Additional analysis: start from end and look for code that doesn't either exit func or continue it somehow
	if( g_filterPostRet )
	{
	}
	*/

	rc = UV_ERR_OK;

error:
	return UV_DEBUG(rc);
}
#endif

//Second pass used to create for DB storage
//Block contains analyzed code
uv_err_t UVD::blockToFunction(UVDAnalyzedBlock *functionBlock, UVDBinaryFunction **functionIn)
{
	UVDBinaryFunction *function = NULL;
	UVDBinaryFunctionShared *functionShared = NULL;
	//Since ownership will be xferred to object, we need to map it twice
	UVDDataChunk *functionBlockDataChunk = NULL;
	//UVDDataChunk *functionDataChunk = NULL;
	//UVDDataChunk *functionInstanceDataChunk = NULL;
	UVDBinaryFunctionInstance *functionInstance = NULL;

	uv_assert_ret(functionBlock);
	uv_assert_ret(functionIn);
	
	uv_assert_err_ret(functionBlock->getDataChunk(&functionBlockDataChunk));
	uv_assert_ret(functionBlockDataChunk)
	//uv_assert_err_ret(functionBlock->getDataChunk(&functionDataChunk));
	//uv_assert_err_ret(functionBlock->getDataChunk(&functionInstanceDataChunk));
	//uv_assert_ret(functionDataChunk);
	//uv_assert_ret(functionInstanceDataChunk);
	//uv_assert_ret(functionDataChunk->m_data);
	//uv_assert_ret(functionInstanceDataChunk->m_data);

	function = new UVDBinaryFunction();
	uv_assert_ret(function);
	//uv_assert_err_ret(function->init());

	functionShared = new UVDBinaryFunctionShared();
	printf_debug("new UVDBinaryFunctionShared: 0x%.8X\n", (unsigned int)function);	
	uv_assert_ret(functionShared);

	uint32_t minAddress = 0;
	uv_assert_err_ret(functionBlock->getMinAddress(minAddress));
	function->m_offset = minAddress;
	
	//The true name of the function is unknown
	//This name is stored as a way to figure out the real name vs the current symbol name 
	functionShared->m_name = "";
	functionShared->m_description = "Automatically generated";	

	//Create the single known instance of this function
	uv_assert_err_ret(UVDBinaryFunctionInstance::getUVDBinaryFunctionInstance(&functionInstance));
	uv_assert_ret(functionInstance);
	functionInstance->m_symbolAddress = UVDRelocatableElement(minAddress);
	//Only specific instances get symbol designations
	std::string symbolName;
	uv_assert_err_ret(m_analyzer->m_symbolManager.analyzedSymbolName(minAddress, UVD__SYMBOL_TYPE__FUNCTION, symbolName));
	functionInstance->setSymbolName(symbolName);
	
	//This will perform copy
	uv_assert_err_ret(functionInstance->setData(functionBlockDataChunk));
	//And register it to that particular function
	functionShared->m_representations.push_back(functionInstance);
	
	function->m_shared = functionShared;
	function->m_uvd = this;
	uv_assert_err_ret(functionBlockDataChunk->deepCopy(&function->m_data));
	
	*functionIn = function;

	return UV_ERR_OK;
}

#if 0
uv_err_t UVD::analyzeCode(UVDAnalyzedCode &analyzedCode)
{
	return UV_ERR_OK;
}
#endif

uv_err_t UVD::analyzeStrings()
{
#if 0
	/*
	Do strings(3) like processing to find ROM/string data
	For some formats like ELF, it is possible to get this string table more directly
	*/

	uv_err_t rc = UV_ERR_GENERAL;
	
	UV_ENTER();

	printf_debug_level(UVD_DEBUG_PASSES, "uvd: analyzing strings...\n");
	UVDBenchmark stringAnalysisBenchmark;
	stringAnalysisBenchmark.start();
	
	//For now assume non-coding addresses are ROM data since thats primary motiviation to exclude them
	for( unsigned int i = 0; i < m_noncodingAddresses.size(); ++i )
	{
		UVDAddressRange memLoc = m_noncodingAddresses[i];
		//Do a C string table analysis
		for( unsigned int i = memLoc.m_min_addr; i < memLoc.m_max_addr; )
		{
			unsigned int j = 0;
			for( j = i; j <= memLoc.m_max_addr; )
			{
				int readRaw = 0; 
				char c = 0;
				
				readRaw = m_data->read(j);
				++j;
				
				if( readRaw < 0 )
				{
					UV_DEBUG(rc);
					goto error;
				}
				c = (char)readRaw;
				//null terminator
				if( c == 0 )
				{
					//Add the string
					uv_addr_t endAddr = j - 1;

					//Just insert one reference to each string for now
					//A map
					m_analyzer->m_stringAddresses[i] = new UVDAnalyzedMemoryRange(i, endAddr);
					m_analyzer->m_stringAddresses[i]->insertReference(i, UVD_MEMORY_REFERENCE_CONSTANT | UVD_MEMORY_REFERENCE_STRING);
					break;
				}
			}
			
			//Start is where we left off			
			i = j;
		}
	}
	
	stringAnalysisBenchmark.stop();
	printf_debug_level(UVD_DEBUG_PASSES, "string analysis time: %s\n", stringAnalysisBenchmark.toString().c_str());
	

	rc = UV_ERR_OK;
	
error:
	return UV_DEBUG(rc);
#endif
	return UV_ERR_OK;
}

uv_err_t UVD::constructBlock(UVDAddressRange addressRange, UVDAnalyzedBlock **blockOut)
{
	/*
	TODO: move this to UVDAnalyzedBlock->init()
	*/
	
	UVDAnalyzedBlock *block = NULL;
	UVDAnalyzedCode *analyzedCode = NULL;
	UVDDataChunk *dataChunk = NULL;
	UVDData *data = NULL;
	//uint32_t dataSize = 0;
	
	uv_assert_ret(addressRange.m_space);
	data = addressRange.m_space->m_data;
	uv_addr_t minAddr = addressRange.m_min_addr;
	uv_addr_t maxAddr = addressRange.m_max_addr;
	
	uv_assert_ret(m_config);
	uv_assert_ret(blockOut);
	
	printf_debug("Constructing block 0x%.8X:0x%.8X\n", minAddr, maxAddr);
	uv_assert_ret(data);

	uv_assert_ret(minAddr <= maxAddr);
	//uv_assert_ret(maxAddr <= m_config->m_addr_max);
	//dataSize = maxAddr - minAddr;

	block = new UVDAnalyzedBlock();
	uv_assert_ret(block);
	block->m_addressSpace = addressRange.m_space;
	
	analyzedCode = new UVDAnalyzedCode();
	uv_assert_ret(analyzedCode);
	block->m_code = analyzedCode;

	dataChunk = new UVDDataChunk();
	uv_assert_ret(dataChunk);
	uv_assert_ret(data);
	uv_assert_err_ret(dataChunk->init(data, minAddr, maxAddr));
	uv_assert_ret(dataChunk->m_data);
	analyzedCode->m_dataChunk = dataChunk;

	*blockOut = block;

	return UV_ERR_OK;
}

/*
static int UVDAnalyzedMemorySpaceSorter(UVDAnalyzedMemoryRange *l, UVDAnalyzedMemoryRange *r)
{
	if( l == r )
	{
		return 0;
	}
	if( l == NULL )
	{
		return -1;
	}
	if( r == NULL )
	{
		return 1;
	}
	
	//printf("Comparing: 0x.%.8X to 0x%.8X\n", r->m_min_addr, l-> m_min_addr);
	
	//Both different and non-null: standard comapre
	return r->m_min_addr - l-> m_min_addr;
}
*/

uv_err_t memorySpaceToMemoryLocations(const UVDAnalyzedMemorySpace &space, UVDAnalyzedMemoryRanges& ret)
{
	uv_err_t rc = UV_ERR_GENERAL;

	ret.clear();
	
	//Some way to make insertion copy, but can't remember off of top of head
	//std::copy(ret.begin(), space.begin(), space.end());
	for( UVDAnalyzedMemorySpace::const_iterator iter = space.begin(); iter != space.end(); ++iter )
	{
		UVDAnalyzedMemoryRange *memoryLocation = (*iter).second;
		uv_assert(memoryLocation);
		ret.push_back(memoryLocation);
	}
	//Why does this seg fault?
	//std::sort(ret.begin(), ret.end(), UVDAnalyzedMemorySpaceSorter);
	//printf("Sorted\n");
	//fflush(stdout);

	rc = UV_ERR_OK;
	
error:
	return UV_DEBUG(rc);
}

uv_err_t UVD::constructJumpBlocks(UVDAnalyzedBlock *superblock, UVDAnalyzedMemoryRanges &superblockLocations, UVDAnalyzedMemoryRanges::iterator &iterSuperblock)
{
	//Commented out because its not being used?  What about labeling?
	return UV_ERR_OK;
/*
	uv_err_t rc = UV_ERR_GENERAL;
	uint32_t lastSuperblockLocation = 0;
	uint32_t superblockMinAddress = 0;
	uint32_t superblockMaxAddress = 0;
	UVDAnalyzedBlock *jumpedBlock = NULL;

	printf_debug("\n");
	UV_ENTER();

	uv_assert(superblock);

	uv_assert_err(superblock->getMinAddress(superblockMinAddress));
	uv_assert_err(superblock->getMaxAddress(superblockMaxAddress));
	
	//Skip ahead to where the superblock is
	while( iterSuperblock != superblockLocations.end() )
	{
		UVDAnalyzedMemoryRange *memoryCalled = *iterSuperblock;
		uv_assert(memoryCalled);

		//Do we have at least func start address?
		if( memoryCalled->m_min_addr >= superblockMinAddress )
		{
			//If so, we are done seeking
			break;
		}
		++iterSuperblock;
	}
	//Loop for all called locations in this block
	for( ; iterSuperblock != superblockLocations.end(); ++iterSuperblock )
	{
		UVDAnalyzedMemoryRange *memoryJumped = *iterSuperblock;		
		uint32_t jumpedBlockEnd = 0;
		
		uv_assert(memoryJumped);
		
		//Are we past superblock?
		if( memoryJumped->m_min_addr > superblockMaxAddress )
		{
			break;
		}
		
		//Nothing precedes superblock start, skip block
		if( memoryJumped->m_min_addr == superblockMinAddress )
		{
			continue;
		}

		//Add the block
		jumpedBlockEnd = memoryJumped->m_min_addr - 1;
		uv_assert_err(constructBlock(lastSuperblockLocation, jumpedBlockEnd, &jumpedBlock));
		uv_assert(jumpedBlock);
		superblock->m_blocks.push_back(jumpedBlock);
		lastSuperblockLocation = memoryJumped->m_min_addr;
	}
	
	//Finally, end of function block
	//We want location after last block
	uv_assert_err(constructBlock(lastSuperblockLocation, superblockMaxAddress, &jumpedBlock));
	uv_assert(jumpedBlock);
	superblock->m_blocks.push_back(jumpedBlock);
	
	rc = UV_ERR_OK;
	
error:
	return UV_DEBUG(rc);
*/
}


UVD::UVD()
{
	m_analyzer = NULL;
	m_format = NULL;
	m_config = NULL;
	m_pluginEngine = NULL;
	m_runtime = NULL;
	m_flirt = NULL;
	m_eventEngine = NULL;
}

UVD::~UVD()
{
	deinit();
}

uv_err_t UVD::deinit()
{
	UVD_POKE(this);
	UVD_POKE(&m_runtime);
	UVD_POKE(&m_analyzer);
	UVD_POKE(&m_config);
	UVD_POKE(&m_format);
	UVD_POKE(&m_flirt);
	UVD_POKE(&m_eventEngine);
	UVD_POKE(&m_pluginEngine);

	printf_debug_level(UVD_DEBUG_PASSES, "UVD::deinit, this=0x%08X, g_uvd=0x%08X, m_config=0x%08X, g_config=0x%08X\n",
			(int)this, (int)g_uvd, (int)m_config, (int)g_config);
	if( m_config )
	{
		uv_assert_err_ret(m_config->m_plugin.m_pluginEngine.onUVDDeinit());
	}
	
	//m_data deallocated by UVD engine caller
	
	delete m_analyzer;
	m_analyzer = NULL;

	delete m_format;
	m_format = NULL;

	//To help migrate from global instances
	//At the time of this writing, g_config is deleted during UVDDeinit()
	if( m_config != g_config )
	{
		delete m_config;
		m_config = NULL;
	}
	
	if( g_uvd == this )
	{
		g_uvd = NULL;
	}
	
	return UV_ERR_OK;
}

uv_err_t UVD::getUVDFromFileName(UVD **uvdOut, const std::string &file)
{
	UVD *uvd = NULL;
	
	//Because code needs lots of fixing before this will work
	if( g_uvd )
	{
		printf_error("sorry, only one UVD instance supported at a time (active = 0x%08X)\n", (int)g_uvd);
		return UV_DEBUG(UV_ERR_NOTIMPLEMENTED);
	}

	uvd = new UVD();
	if( !uvd )
	{
		return UV_DEBUG(UV_ERR_GENERAL);
	}
	
	uv_assert_ret(g_config);
	uvd->m_config = g_config;
	
	if( UV_FAILED(uvd->initFromFileName(file)) )
	{
		delete uvd;
		return UV_DEBUG(UV_ERR_GENERAL);
	}	
	g_uvd = uvd;

	uv_assert_ret(uvdOut);
	*uvdOut = uvd;
	return UV_ERR_OK;
}

//Factory function for construction
uv_err_t UVD::getUVDFromData(UVD **uvdOut, UVDData *data)
{
	UVD *uvd = NULL;
	
	if( g_uvd )
	{
		uvd = g_uvd;	
	}
	else
	{
		uvd = new UVD();
		if( !uvd )
		{
			return UV_DEBUG(UV_ERR_GENERAL);
		}
		
		uv_assert_ret(g_config);
		uvd->m_config = g_config;
		
		if( UV_FAILED(uvd->initFromData(data)) )
		{
			delete uvd;
			return UV_DEBUG(UV_ERR_GENERAL);
		}
		
		g_uvd = uvd;
	}

	uv_assert_ret(uvdOut);
	*uvdOut = uvd;
	return UV_ERR_OK;
}

/*
uv_err_t UVD::opcodeDeinit()
{
	return m_opcodeTable->deinit();
}
*/

UVDIterator UVD::begin()
{
	UVDIterator iter;
	
	if( UV_FAILED(begin(iter)) )
	{
		UVD_PRINT_STACK();
	}
	return iter;
}
	
uv_err_t UVD::begin(UVDIterator &iter)
{
	UVDAddressSpace *addressSpace = NULL;
	
	uv_assert_err_ret(m_runtime->getPrimaryExecutableAddressSpace(&addressSpace));
	uv_assert_err_ret(iter.init(this, addressSpace));
	
	return UV_ERR_OK;
}

UVDIterator UVD::begin(uv_addr_t offset)
{
	UVDIterator iter;
	UVDAddressSpace *addressSpace = NULL;
	
	UV_DEBUG(m_runtime->getPrimaryExecutableAddressSpace(&addressSpace));
	UV_DEBUG(iter.init(this, UVDAddress(offset, addressSpace), 0));

	return iter;
}

uv_err_t UVD::begin(UVDAddress address, UVDIterator &iter)
{
	uv_assert_err_ret(iter.init(this, address, 0));

	return UV_ERR_OK;
}

/*
UVDIterator UVD::begin(UVDData *data)
{
	UVDIterator iter = begin();
	iter.m_data = data;
	return iter;
}
*/

UVDIterator UVD::end()
{
	UVDIterator iter;

	UV_DEBUG(end(iter));
	return iter;
}

uv_err_t UVD::end(UVDIterator &iter)
{
	//Pos is "next position"
	//Size is first invalid position
	//UVDIterator iter;
	
	uv_assert_err_ret(UVDIterator::getEnd(this, iter));
	
	//This will work fine unless we fill up the entire address space
	//UV_DEBUG(iter.init(this));
	//iter.m_data = m_data;
	//The key part
	//UV_DEBUG(iter.makeEnd());
	return UV_ERR_OK;
}

/*
UVDIterator UVD::end(UVDData *data)
{
	UVDIterator iter = end();
	iter.m_data = data;
	return iter;
}
*/

UVDInstructionIterator UVD::instructionBegin()
{
	UVDInstructionIterator iter;
	UV_DEBUG(instructionBegin(iter));
	return iter;
}

uv_err_t UVD::instructionBegin(UVDInstructionIterator &iter)
{
	UVDAddressSpace *addressSpace = NULL;
	
	uv_assert_err_ret(m_runtime->getPrimaryExecutableAddressSpace(&addressSpace));	
	uv_assert_ret(addressSpace);
	uv_assert_err_ret(iter.init(this, addressSpace));

	return UV_ERR_OK;	
}

UVDInstructionIterator UVD::instructionEnd()
{
	UVDInstructionIterator iter;

	UV_DEBUG(instructionEnd(iter));
	return iter;
}

uv_err_t UVD::instructionEnd(UVDInstructionIterator &iter)
{
	//This will work fine unless we fill up the entire address space
	//uv_assert_err_ret(iter.init(this));
	//iter.m_data = m_data;
	//The key part
	//uv_assert_err_ret(iter.makeEnd());
	uv_assert_err_ret(UVDInstructionIterator::getEnd(this, iter));
	return UV_ERR_OK;
}

//FIXME: doesn't split
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> ret;
	ret.push_back(s);
	return ret;
}

uv_err_t UVD::stringListAppend(UVDInstruction *inst, std::vector<std::string> &list)
{
	//eh this is odd
	//I don't think an instruction can print multiple lines
	std::string instructionLines;
	
	uv_assert_ret(inst);
	
	printf_debug("printing disasm\n");
	uv_assert_err_ret(inst->print_disasm(instructionLines));
	printf_debug("printed disasm\n");
	std::vector<std::string> toAppend;

	toAppend = split(instructionLines, '\n', true);
	for( std::vector<std::string>::size_type i = 0; i < toAppend.size(); ++i )
	{
		list.insert(list.end(), toAppend[i]);
	}
	
	return UV_ERR_OK;
}

/*
uv_err_t UVD::createAnalysisDir(const std::string &file, const std::string &outputDir)
{
	std::string oldDir;
	
	oldDir = m_config->m_analysisDir;
	m_config->m_analysisDir = outputDir;
	uv_assert_err_ret(createAnalysisDir());
	m_config->m_analysisDir = oldDir;

	return UV_ERR_OK;
}

uv_err_t UVD::createAnalysisDir()
{
	uint32_t oldAnalysisOnly = 0;
	//Due to decompile hack
	std::string discard;
	
	uv_assert_ret(m_config);
	oldAnalysisOnly = m_config->m_analysisOnly;
	m_config->m_analysisOnly = 1;
	//Sorta hackish, do some cleanup later
	//Lang is relativly unimportant, assembly indicates don't do any high level analysis
	//(although it is ignored for now anyway)
	uv_assert_err_ret(analyze());
	m_config->m_analysisOnly = oldAnalysisOnly;
	
	return UV_ERR_OK;
}
*/

uv_err_t UVD::disassemble(std::string &output)
{
	//TODO: set config directive here
	return UV_DEBUG(decompile(output));
}

uv_err_t UVD::decompile(std::string &output)
{
	UVDBenchmark decompileBenchmark;
	UVDIterator iterBegin;
	UVDIterator iterEnd;


	decompileBenchmark.start();

	//Most of program time should be spent here
	uv_assert_err_ret(analyze());
	//Until we can do better
	setDestinationLanguage(UVD_LANGUAGE_ASSEMBLY);
	//And print
	uv_assert_err_ret(begin(iterBegin));
	uv_assert_err_ret(end(iterEnd));
	uv_assert_err_ret(printRangeCore(iterBegin, iterEnd, output));

	printf_debug_level(UVD_DEBUG_PASSES, "decompile: done\n");
	decompileBenchmark.stop();
	printf_debug_level(UVD_DEBUG_PASSES, "decompile time: %s\n", decompileBenchmark.toString().c_str());
	
//printf("DEBUG BREAK\n");
//exit(1);

	return UV_ERR_OK;
}

uv_err_t UVD::printRange(uv_addr_t start, uv_addr_t end, uint32_t destinationLanguage, std::string &output)
{
	return UV_DEBUG(UV_ERR_NOTIMPLEMENTED);
}

uv_err_t UVD::setDestinationLanguage(uint32_t destinationLanguage)
{
	UVDCompiler *compiler = NULL;

	switch( destinationLanguage )
	{
	case UVD_LANGUAGE_ASSEMBLY:
		compiler = new UVDCompilerAssembly();
		break;
	default:
		printf_error("Unknown destination langauge: 0x%.4X (%d)\n", destinationLanguage, destinationLanguage);
		return UV_DEBUG(UV_ERR_GENERAL);
	};
	uv_assert_ret(compiler);
	//Set a default compiler to generate code for
	//How this is set will probably change drastically in the future
	uv_assert_ret(m_format);
	uv_assert_err_ret(m_format->setCompiler(compiler));
	return UV_ERR_OK;
}

uv_err_t UVD::printRangeCore(UVDIterator iterBegin, UVDIterator iterEnd, std::string &output)
{
	UVDIterator iter;
	//UVDIterator iterEnd;
	int printPercentage = 1;
	int printNext = printPercentage;
	uint32_t analyzedBytes = 0;
	int verbose_old = 0;

	uv_assert_ret(m_config);
	//FIXME: this should be delta, not single...w/e
	uv_assert_err_ret(iterBegin.m_addressSpace->getNumberAnalyzedBytes(&analyzedBytes));
	uv_assert_ret(analyzedBytes != 0);
	verbose_old = m_config->m_verbose;
	m_config->m_verbose = m_config->m_verbose_printing;

	printf_debug_level(UVD_DEBUG_PASSES, "decompile: printing...\n");
	UVDBenchmark decompilePrintBenchmark;
	decompilePrintBenchmark.start();
	iter = iterBegin;

	//Due to the huge number of concatenations
#ifdef USING_ROPE
	__gnu_cxx::crope outputRope;
#else
	output.clear();
#endif //USING_ROPE
	int iterations = 0;
	//FIXME: what if we misalign by accident and surpass?
	//need to add some check for that
	//maybe we should do <
	while( iter != iterEnd )
	{
		std::string line;
		uint32_t startPos = iter.getPosition();

		++iterations;				
		printf_debug("\n\n\n");
		printf_debug("Iteration loop iteration\n");		

		int curPercent = 100 * startPos / analyzedBytes;
		if( curPercent >= printNext )
		{
			uint64_t delta = getTimingMicroseconds() - decompilePrintBenchmark.getStart();
			double iterationTime = 1.0 * delta / iterations;
			
			printf_debug_level(UVD_DEBUG_SUMMARY, "uvd: printing: %d %% (us / record: %.2lf)\n", curPercent, iterationTime);
			printNext += printPercentage;
		}

		uv_assert_err_ret(iter.getCurrent(line));
		printf_debug("Line (0x%.8X): %s\n", iter.getPosition(), line.c_str());

		//This didn't help for the bottleneck under investigation
#if USING_ROPE
		outputRope += (
#else
		output +=
#endif //USING_ROPE
				line + "\n"
#if USING_ROPE
				).c_str()
#endif //USING_ROPE
				;				
		printf_debug("\n");
		uv_assert_err_ret(iter.next());
	}
#ifdef USING_ROPE
	output = outputRope.c_str();
	outputRope.clear();
#endif //USING_ROPE
	
	/*
	drop for now during architecture abstraction
	if( m_config->m_print_used )
	{
		m_architecture->m_opcodeTable->usedStats();
	}
	*/

	decompilePrintBenchmark.stop();
	printf_debug_level(UVD_DEBUG_PASSES, "decompile print time (%d records): %s\n", iterations, decompilePrintBenchmark.toString().c_str());

	m_config->m_verbose = verbose_old;

	return UV_ERR_OK;
}

/*
uv_err_t UVD::changeConfig(UVDConfig *config)
{
	if( m_config )
	{
		delete m_config;
	}
	m_config = config;
	return UV_ERR_OK;
}
*/

uv_err_t UVD::setOutputFormatting(UVDFormat *format)
{
	uv_assert_ret(format);

	//We could consider moving some of the settings in if they are undefined by the new object if an old exists
	delete m_format;
	m_format = format;

	return UV_ERR_OK;
}

UVD *UVDGetUVD()
{
	return g_uvd;
}

