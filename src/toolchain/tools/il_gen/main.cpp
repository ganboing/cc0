#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <ctime>
#include <boost/archive/xml_oarchive.hpp>

#include <core/Core.h>
#include <frontend/c/CSourceParser.h>
#include <backend/i0/I0CodeGenerator.h>
#include <binary/flat/FlatFileWriter.h>
#include <core/Symbol/SymbolAddressAllocator.h>
#include <core/Pass/ConstantPropagation.h>
#include <core/Pass/TypeDeduction.h>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"
#include "core/Serialization/ObjFormat.h"

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

void print_usage(char *) {
	printf("cc0(il_gen) - A c0 compiler which generates i0 code.\n"
			"\n"
			"Usage: \n"
			"    il_gen [-g|--debug] infile [-o outfile] [-h|--help]\n"
			"\n"
			"\n"
			"Options:\n"
			"--debug, -g\n"
			"        Output debugging information.\n"
			"-o\n"
			"        Output to specified file"
			"\n");

	return;
}

int main(int argc, char **argv) {
	CompilationContext *context = CompilationContext::GetInstance();

	// context->TextStart =  0x400000000;
	// context->DataStart =  0x400004000;
	// context->RDataStart = 0x400008000;
	// Use macros from the sys_config.h
	context->TextStart = I0_CODE_BEGIN;
	context->DataStart = I0_CODE_BEGIN + 0x4000;
	context->RDataStart = I0_CODE_BEGIN + 0x8000;

	//NOTE: Currently, all global variables are put in the bss section and are NOT initialized with zeros, the data/rdata is not used.
	// context->BssStart =   0x440000000;
	context->BssStart = AMR_OFFSET_BEGIN;

	// NOTE: default targe code type
	// Only CODE_TYPE_I0 is supported
	CompilationContext::GetInstance()->CodeType = CODE_TYPE_I0;

	::std::string c0_obj_file;
	::std::vector<char*> cpp_options;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
				c0_obj_file = argv[++i];
			}
		} else if (!strncmp("-D", argv[i], 2)) {
			cpp_options.push_back(argv[i]);
		} else if ((strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-g") == 0)) {
			CompilationContext::GetInstance()->Debug = true;
		} else if ((strcmp(argv[i], "--help") == 0) || strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 1;
		} else {
			if (CompilationContext::GetInstance()->InputFiles.size()) {
				::std::cerr << "multiple input file applied!\n";
				print_usage(argv[0]);
				return 1;
			}
			CompilationContext::GetInstance()->InputFiles.push_back(argv[i]);
		}
	}

	if (CompilationContext::GetInstance()->InputFiles.size() == 0) {
		std::cerr << "please specify input files\n";
		return 1;
	}

	if (c0_obj_file.size() == 0) {
		c0_obj_file = "a.c0obj";
		if (CompilationContext::GetInstance()->Debug) {
			std::cout << "output file default to " << c0_obj_file << "\n";
		}
	}

	CompilationContext::GetInstance()->CompileOnly = true;

	ILProgram *il = NULL;

	std::string& inputFile = CompilationContext::GetInstance()->InputFiles.front();

	::std::stringstream tmpFileName_impl;
	tmpFileName_impl << "/tmp/cc0_cpp_" << getpid() << "_" << time(NULL);
	::std::string tmpFileName(tmpFileName_impl.str());
	std::cout << "temp file is: " << tmpFileName << "\n";

	context->CurrentFileName = inputFile;

	::std::stringstream cpp_cmdline;
	cpp_cmdline << "cpp ";
	for (::std::vector<char*>::iterator i = cpp_options.begin(), iE = cpp_options.end(); i != iE; ++i) {
		cpp_cmdline << (*i) << " ";
	}
	cpp_cmdline << inputFile << " -o " << tmpFileName;

	if (CompilationContext::GetInstance()->Debug) {
		::std::cout << "invoking " << cpp_cmdline.str() << "\n";
	}
	if (system(cpp_cmdline.str().c_str()) != 0) {
		return -1;
	}

	if (CompilationContext::GetInstance()->Debug) {
		printf("--------------------------------------\n");
		printf("parsing...\n");
	}

	CSourceParser *frontend = new CSourceParser();
	frontend->Parse(tmpFileName);

	// Note: leave tmpFile for user to check
	// remove(tmpFileName);

	if (CompilationContext::GetInstance()->Debug) {
		printf("--------------------------------------\n");
		printf("ConstantPropagation...\n");
	}

	ConstantPropagation *constantPropagation = new ConstantPropagation();
	context->CodeDom->Accept(constantPropagation);

	if (CompilationContext::GetInstance()->Debug) {
		printf("--------------------------------------\n");
		printf("ConstantPropagation...\n");
	}

	TypeDeduction *typeDeduction = new TypeDeduction();
	context->CodeDom->Accept(typeDeduction);

	if (CompilationContext::GetInstance()->Debug) {
		printf("--------------------------------------\n");
		printf("codeDom Dump:\n");
		ExpressionTreeDumper *codeDomDump = new ExpressionTreeDumper();

		context->CodeDom->Accept(codeDomDump);
	}

	ILGenerator *ilgen = new ILGenerator();
	context->CodeDom->Accept(ilgen);

	il = ilgen->GetILProgram();
	if (il == NULL) {
		return -1;
	}
	context->IL = il;

	{
		CC0Obj obj(context->CodeDom, context->IL);
		::std::ofstream filestream(c0_obj_file.c_str());
		::boost::archive::xml_oarchive c0_obj_archive(filestream);
		c0_obj_archive & BOOST_SERIALIZATION_NVP(obj);
	}

	return 0;
}
