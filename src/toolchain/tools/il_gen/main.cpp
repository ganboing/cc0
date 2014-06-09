#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/filesystem/path.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <core/Core.h>
#include <frontend/c/CSourceParser.h>
// #include <backend/disa/DisaCodeGenerator.h>
// #include <backend/disa/DisaAssemblyParser.h>
#include <backend/i0/I0CodeGenerator.h>
#include <binary/flat/FlatFileWriter.h>
#include <core/Symbol/SymbolAddressAllocator.h>
#include <core/Pass/ConstantPropagation.h>
#include <core/Pass/TypeDeduction.h>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

void print_usage(char *cmd)
{
    printf(
"cc0(il_gen) - A c0 compiler which generates i0 code.\n"
"\n"
"Usage: \n"
"    il_gen [-g] infile [-o outfile] [-h|--help]\n"
"\n"
"\n"
"Options:\n"
"--debug, -g\n"
"        Output debugging information.\n"
"-o\n"
"        Output to specified file"
"\n"
    );

    return;
}

int main(int argc, char **argv)
{

    bool codeTypeDefined = false;

    CompilationContext *context = CompilationContext::GetInstance();

    // context->TextStart =  0x400000000;
    // context->DataStart =  0x400004000;
    // context->RDataStart = 0x400008000;
    // Use macros from the sys_config.h
    context->TextStart =  I0_CODE_BEGIN;
    context->DataStart =  I0_CODE_BEGIN + 0x4000;
    context->RDataStart = I0_CODE_BEGIN + 0x8000;

    //NOTE: Currently, all global variables are put in the bss section and are NOT initialized with zeros, the data/rdata is not used.
    // context->BssStart =   0x440000000;
    context->BssStart =   AMR_OFFSET_BEGIN;

    // NOTE: default targe code type
    // Only CODE_TYPE_I0 is supported
    CompilationContext::GetInstance()->CodeType = CODE_TYPE_I0;
    codeTypeDefined = true;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
            	CompilationContext::GetInstance()->OutputFile = argv[++i];
			} else {
				CompilationContext::GetInstance()->OutputFile = "";
			}
        }
        else if( (strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-g") == 0) )
        {
            CompilationContext::GetInstance()->Debug = true;
        }
        /*
        else if (strcmp(argv[i], "--i0") == 0)
        {
            if (codeTypeDefined) {
                printf("--i0 and --disa can not be used at the same time.\n"
                        "Specify one code type only.\n");
                return -1;
            }
            CompilationContext::GetInstance()->CodeType = CODE_TYPE_I0;
            codeTypeDefined = true;
        }
        else if (strcmp(argv[i], "--disa") == 0)
        {
            if (codeTypeDefined) {
                printf("--i0 and --disa can not be used at the same time.\n"
                        "Specify one code type only.\n");
                return -11;
            }

            CompilationContext::GetInstance()->CodeType = CODE_TYPE_DISA;
            codeTypeDefined = true;
        }
        */
        else if ( (strcmp(argv[i], "--help") == 0) || strcmp(argv[i], "-h") == 0 )
        {
            print_usage(argv[0]);
            return 1;
        }
        else
        {
        	if(CompilationContext::GetInstance()->InputFiles.size())
        	{
        		print_usage(argv[0]);
        		return 1;
        	}
            CompilationContext::GetInstance()->InputFiles.push_back(argv[i]);
        }
    }

    if(CompilationContext::GetInstance()->InputFiles.size() == 0)
    {
    	std::cerr<< "please specify input files\n";
    	return 1;
    }

    if(CompilationContext::GetInstance()->OutputFile.size() == 0)
    {
    	::boost::filesystem::path output_file_path(CompilationContext::GetInstance()->InputFiles[0]);
    	::std::cout<<"file extension == " << output_file_path.extension() << "\n";
    	output_file_path.replace_extension(".c0obj");
    	CompilationContext::GetInstance()->OutputFile = output_file_path.c_str();
    	::std::cout<<"will output to " << output_file_path <<"\n";
    }

    CompilationContext::GetInstance()->CompileOnly = true;

    ILProgram *il = NULL;

    std::vector<std::string> &inputFiles = CompilationContext::GetInstance()->InputFiles;

    for(std::vector<std::string>::iterator it = inputFiles.begin(); it != inputFiles.end(); ++it)
    {
        std::string inputFile = CompilationContext::GetInstance()->InputFiles.front();
        std::string fileExt = ::boost::filesystem::path(inputFile).extension().c_str();
        if(fileExt == ".c" || fileExt == ".c0")
        {
        	std::string tmpFileName = inputFile + ".tmp";

            // tmpnam(tmpFileName);
            std::cout<< "temp file is: " << tmpFileName << "\n";

            context->CurrentFileName = inputFile;

            std::string cmdline = "cpp " + inputFile + " -o " + tmpFileName;
            if(system(cmdline.c_str()) != 0)
            {
               return -1;
            }

            if(CompilationContext::GetInstance()->Debug)
            {
                printf("--------------------------------------\n");
                printf("parsing...\n");
            }

            CSourceParser *frontend = new CSourceParser();
            frontend->Parse(tmpFileName);

            // Note: leave tmpFile for user to check
            // remove(tmpFileName);

            if(CompilationContext::GetInstance()->Debug)
            {
                printf("--------------------------------------\n");
                printf("ConstantPropagation...\n");
            }

            ConstantPropagation *constantPropagation = new ConstantPropagation();
            context->CodeDom->Accept(constantPropagation);

            if(CompilationContext::GetInstance()->Debug)
            {
                printf("--------------------------------------\n");
                printf("ConstantPropagation...\n");
            }

            TypeDeduction *typeDeduction = new TypeDeduction();
            context->CodeDom->Accept(typeDeduction);

            if(CompilationContext::GetInstance()->Debug)
            {
                printf("--------------------------------------\n");
                printf("codeDom Dump:\n");
                ExpressionTreeDumper *codeDomDump = new ExpressionTreeDumper();

                context->CodeDom->Accept(codeDomDump);
            }

            ILGenerator *ilgen = new ILGenerator();
            context->CodeDom->Accept(ilgen);

            il = ilgen->GetILProgram();
        }
    }

    if(il == NULL)
    {
        return -1;
    }

    context->IL = il;

    {
    	::std::ofstream c0_obj_file(context->OutputFile.c_str());
    	::boost::archive::xml_oarchive c0_obj_archive(c0_obj_file);
    	c0_obj_archive & BOOST_SERIALIZATION_NVP(context);
    }

    return 0;
}
