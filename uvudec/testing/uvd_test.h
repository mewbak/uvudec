/*
UVNet Universal Decompiler (uvudec)
Copyright 2010 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#ifndef UVD_TEST_H
#define UVD_TEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define UVCPPUNIT_ASSERT(x)			CPPUNIT_ASSERT(UV_SUCCEEDED(x))

class UVDUnitTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(UVDUnitTest);
	CPPUNIT_TEST(versionTest);
	CPPUNIT_TEST(defaultDecompileFileTest);
	CPPUNIT_TEST(initDeinitTest);
	CPPUNIT_TEST(versionArgTest);
	CPPUNIT_TEST(helpArgTest);
	CPPUNIT_TEST(engineInitTest);
	//CPPUNIT_TEST(analysisDirTest);
	CPPUNIT_TEST(disassembleTest);
	CPPUNIT_TEST(disassembleRangeTestDeliminators);
	CPPUNIT_TEST(disassembleRangeTestDefaultEquivilence);
	CPPUNIT_TEST(disassembleRangeTestComplex);
	CPPUNIT_TEST(uvudecBasicRun);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp(void);
		void tearDown(void);

	protected:
		/*
		Checks our exe compiled version aginst the library compiled version
		They should be equal
		*/
		void versionTest(void);
		/*
		Make sure that our default decompile file is accessible
		It is required for engine initialization
		*/
		void defaultDecompileFileTest(void);
		/*
		Early initialization
		Logging, argument parsing structures
		Should NOT initialize the actual decompiler engine
		*/
		void initDeinitTest(void);
		/*
		Test the "--version" option
		*/
		void versionArgTest(void);
		/*
		Test the "--help" option
		*/
		void helpArgTest(void);
		/*
		Perform a full engine init, but don't do any analysis
		*/
		void engineInitTest(void);
		/*
		Generate analysis output such as object files
		*/
		void analysisDirTest(void);
		/*
		Disassemble the default binary
		*/
		void disassembleTest(void);
		/*
		Disassemble the default binary with inclusion/exclusion ranges
		*/
		void disassembleRangeTestDeliminators(void);
		void disassembleRangeTestDefaultEquivilence(void);
		void disassembleRangeTestComplex(void);
		/*
		Actually calls uvudec's uvmain using the hooks
		Does a basic test where as most of hte thorough test test libuvudec rather than what the uvudec exe can do
		*/
		void uvudecBasicRun(void);
	
	private:
		//Initize and verify that we don't have any errors running these args
		//program name does not need to be supplied
		void generalDisassemble(const std::vector<std::string> &args);
		void generalDisassemble(const std::vector<std::string> &args, std::string &output);
};

#endif
