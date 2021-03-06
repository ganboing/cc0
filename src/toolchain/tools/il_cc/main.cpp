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
#include "core/Serialization/ObjFormat.h"

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

void DumpScopeTypes(SymbolScope *scope, std::ofstream &dump, std::string prefix)
{
    char buffer[100];
    dump << prefix << "SCOPE " << scope << " type " << scope->GetScopeKind() << " {" << std::endl;

    std::string cur_prefix = prefix + "  ";

    for(std::map<std::string, Symbol *>::iterator it = scope->GetSymbolTable()->begin(); it != scope->GetSymbolTable()->end(); ++it)
    {
        Symbol *symbol = it->second;
        // if (typeid(*(symbol->DeclType)) == typeid(FunctionType) || scope->GetScopeKind() == SymbolScope::Global)
        {
        	dump << cur_prefix <<
        			symbol->Name << "\t" <<
        			symbol->DeclType->ToString() << "\t" <<
        			std::endl;
        }
    }

    for(std::vector<SymbolScope *>::iterator it = scope->GetChildScopes()->begin(); it != scope->GetChildScopes()->end(); ++it)
    {
        SymbolScope *cs = *it;
        DumpScopeTypes(cs, dump, cur_prefix);
    }

    dump << prefix << "}" << std::endl;
}


void DumpScope(SymbolScope *scope, std::ofstream &dump)
{
    char buffer[100];

    for(std::map<std::string, Symbol *>::iterator it = scope->GetSymbolTable()->begin(); it != scope->GetSymbolTable()->end(); ++it)
    {
        Symbol *symbol = it->second;
        if (typeid(*(symbol->DeclType)) == typeid(FunctionType) || scope->GetScopeKind() == SymbolScope::Global)
        {
            sprintf(buffer, "%0llX\t%s", (long long)symbol->Address, symbol->Name.c_str());
            dump << buffer << std::endl;
        }
    }

    for(std::vector<SymbolScope *>::iterator it = scope->GetChildScopes()->begin(); it != scope->GetChildScopes()->end(); ++it)
    {
        SymbolScope *cs = *it;
        DumpScope(cs, dump);
    }
}
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

    ::std::string c0_obj_file;
    bool is_debug(false);

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
				context->OutputFile = argv[++i];
			} else {
				context->OutputFile = "a.bin";
			}
        }
        else if( (strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-g") == 0) )
        {
            CompilationContext::GetInstance()->Debug = true;
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

    if(context->OutputFile.size() == 0)
    {
    	std::cerr << "please specify output file\n";
    	return 1;
    }
    {
    	CC0Obj obj;
    	::std::ifstream filestream(c0_obj_file.c_str());
    	::boost::archive::xml_iarchive c0_obj_archive(filestream);
    	c0_obj_archive & BOOST_SERIALIZATION_NVP(obj);
    	context->IL = obj.second;
    	context->CodeDom = obj.first;
    }

    SymbolScope::__SetRootScopt(context->IL);

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
    if((CompilationContext::GetInstance()->Debug || CompilationContext::GetInstance()->CompileOnly) && il != NULL)
    {
        // printf("--------------------------------------\n");
        // printf("Optimized IL:\n");
        std::string baseFileName = CompilationContext::GetInstance()->OutputFile;
        int pos = baseFileName.find_last_of(".");
        if(pos != -1)
        {
            baseFileName = baseFileName.substr(0, pos) + ".opt.il";
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

    std::string baseFileName = CompilationContext::GetInstance()->OutputFile;
    int pos = baseFileName.find_last_of(".");
    if(pos != -1)
    {
        baseFileName = baseFileName.substr(0, pos);
    }
    std::string mapFileName = baseFileName + ".var";

    std::ofstream mapdump(mapFileName.c_str());
    DumpScopeTypes(SymbolScope::GetRootScope(), mapdump, "");
    mapdump.close();

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


        std::ofstream mapdump(mapFileName.c_str());
        DumpScope(SymbolScope::GetRootScope(), mapdump);
        mapdump.close();
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
