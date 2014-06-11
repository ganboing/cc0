#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/filesystem/path.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <core/Core.h>
#include <frontend/c/CSourceParser.h>
#include <backend/i0/I0CodeGenerator.h>
#include <binary/flat/FlatFileWriter.h>
#include <core/Symbol/SymbolAddressAllocator.h>
#include <core/Pass/ConstantPropagation.h>
#include <core/Pass/TypeDeduction.h>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

void print_usage(char *)
{
    printf(
"cc0(il_cc) - A c0 compiler which generates i0 code.\n"
"\n"
"Usage: \n"
"    il_cc [-g|--debug] [-h|--help] infile -o outfile\n"
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

    ::std::string c0_obj_file;
    ::std::string c0_bin_file;
    bool is_debug(false);

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
				c0_bin_file = argv[++i];
			} else {
				c0_bin_file = "";
			}
        }
        else if( (strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-g") == 0) )
        {
            is_debug = true;
        }
        else if ( (strcmp(argv[i], "--help") == 0) || strcmp(argv[i], "-h") == 0 )
        {
            print_usage(argv[0]);
            return 1;
        }
        else
        {
        	if(c0_obj_file.size())
        	{
        		print_usage(argv[0]);
        		return 1;
        	}
        	c0_obj_file = argv[i];
        }
    }

    if(c0_obj_file.size() == 0)
    {
    	std::cerr<< "please specify input files\n";
    	return 1;
    }

    if(c0_bin_file.size() == 0)
    {
    	::boost::filesystem::path output_file_path(c0_obj_file);
    	output_file_path.replace_extension(".bin");
    	c0_bin_file = output_file_path.string();
    	if(is_debug)
    	{
    		std::cout<< "output file is " << CompilationContext::GetInstance()->OutputFile << "\n";
    	}
    }

    CompilationContext* context = NULL;
    {
    	::std::ifstream filestream(c0_obj_file.c_str());
    	::boost::archive::xml_iarchive c0_obj_archive(filestream);
    	c0_obj_archive & BOOST_SERIALIZATION_NVP(context);
    }
    CompilationContext::__SetInstance(context);

    context->Debug = is_debug;
    context->OutputFile = c0_bin_file;

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

    SymbolScope::__SetRootScopt(context->IL);

    std::vector<std::string> &inputFiles = CompilationContext::GetInstance()->InputFiles;

    ILProgram* il = context->IL;

    if((CompilationContext::GetInstance()->Debug || CompilationContext::GetInstance()->CompileOnly) && il != NULL)
    {
        std::string baseFileName = CompilationContext::GetInstance()->OutputFile;
        int pos = baseFileName.find_last_of(".");
        if(pos != -1)
        {
            baseFileName = baseFileName.substr(0, pos) + ".il";
        }

        std::ofstream ildump(baseFileName.c_str());
        for(std::vector<ILClass *>::iterator cit = il->Claases.begin(); cit != il->Claases.end(); ++cit)
        {
            ILClass *c = *cit;

            ildump << "class " <<  c->ClassSymbol->Name << std::endl << "{" << std::endl;

            for(std::vector<ILFunction *>::iterator fit = c->Functions.begin(); fit != c->Functions.end(); ++fit)
            {
                ILFunction *f = *fit;
                ildump << "    function " <<  f->FunctionSymbol->Name << std::endl << "    {" << std::endl;
                for(std::vector<IL>::iterator iit = f->Body.begin(); iit != f->Body.end(); ++iit)
                {
                    IL &il = *iit;
                    if(il.Opcode == IL::Label)
                    {
                        ildump << "        " << il.ToString() << std::endl;
                    }
                    else
                    {
                        ildump << "            " << il.ToString() << std::endl;
                    }
                }
                ildump << "    }" << std::endl;
            }
            ildump << "}" << std::endl;
        }

        ildump.close();
    }

    context->IL = il;

    // TODO: Optimize the IL
    ILOptimizer *ilopt = NULL;

    ilopt = new ILOptimizer();

    context->IL = ilopt->Optimize(il);

    // print optimized IL
    il = context->IL;

    std::string baseFileName = CompilationContext::GetInstance()->OutputFile;
    int pos = baseFileName.find_last_of(".");
    if(pos != -1)
    {
        baseFileName = baseFileName.substr(0, pos);
    }
    std::string mapFileName = baseFileName + ".var";

    // if it is for -c, return now
    if(CompilationContext::GetInstance()->CompileOnly) {
        // TODO: dump the variable table
        return 0;
    }

    CodeGenerator *codegen = NULL;
    //Generate assembly code from IL
    /* if (CompilationContext::GetInstance()->CodeType == CODE_TYPE_DISA) {
        //Generate DISA assembly code from IL
        codegen = new DisaCodeGenerator();
        } else */
    if (CompilationContext::GetInstance()->CodeType == CODE_TYPE_I0) {
        codegen = new I0CodeGenerator();
    } else {
        printf("Error: unsupported CodeType.\n");
        return -1;
    }

    codegen->Generate(context->IL);


    for(std::vector<std::string>::iterator it = inputFiles.begin(); it != inputFiles.end(); ++it)
    {
        std::string inputFile = context->InputFiles.front();
        std::string fileExt = inputFile.substr(inputFile.find_last_of(".") + 1);
        if(fileExt == "s")
        {
            SourceParser *parser = NULL;
            /* if (CompilationContext::GetInstance()->CodeType == CODE_TYPE_DISA) {
                parser = new DisaAssemblyParser();
            } else
            */
            {
                // TODO: support i0 assembly
                printf(".s file is not supported for i0.\n");
                return -1;
            }
            parser->Parse(inputFile);
        } else if(fileExt == "c") {
            printf("WARNING: It is recommended to use .c0 instead of .c as the source file extension for c0 programs.\n");
        }
    }

    if((CompilationContext::GetInstance()->Debug || CompilationContext::GetInstance()->CompileOnly))
    {
        std::string baseFileName = CompilationContext::GetInstance()->OutputFile;
        std::string dumpFileName, mapFileName;
        int pos = baseFileName.find_last_of(".");
        if(pos != -1)
        {
            baseFileName = baseFileName.substr(0, pos);
        }

        dumpFileName = baseFileName + ".objdump";
        mapFileName = baseFileName + ".map";

        std::ofstream objdump(dumpFileName.c_str());

        int64_t currentText = context->TextStart;
        for(std::vector<TargetInstruction *>::iterator iit = context->Target->Code.begin(); iit != context->Target->Code.end(); ++iit)
        {
            TargetInstruction *inst = *iit;
            char buffer[32];
            sprintf(buffer, "%0llX> \t", (long long)currentText);
            objdump << buffer << inst->ToString().c_str() << std::endl;
            currentText += inst->GetLength();
        }
        objdump.close();

    }

    printf("Maximum stack frame size: 0x%llX\n", (long long )(context->MaxStackFrame));

    // TODO: Optimize the assembly code
    TargetOptimizer *targetOpt = NULL;


    char *textBuf = new char[0x100000];
    int64_t textSize = 0;
    for(std::vector<TargetInstruction *>::iterator it = context->Target->Code.begin(); it != context->Target->Code.end(); ++it)
    {
        TargetInstruction *inst = *it;
        inst->Encode(&textBuf[textSize]);
        textSize += inst->GetLength();
    }

    // Write the binary into file
    std::string outputFile = CompilationContext::GetInstance()->OutputFile;
    BinaryWriter *binwt = new FlatFileWriter();

    std::vector<SectionInfo> sections;
    SectionInfo textSection;
    textSection.Name = ".text";
    textSection.RawData = textBuf;
    textSection.RawDataSize = textSize;
    textSection.VirtualBase = context->TextStart;
    textSection.VirtualSize = textSize;
    sections.push_back(textSection);

    binwt->WriteBinaryFile(context->OutputFile, &sections, context->TextStart);

    return 0;

}