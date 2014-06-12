#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
/*
::std::vector<Type*>* generate_Type_test_case() {
	::std::vector<Declaration*>* decl_arr = new ::std::vector<Declaration*>();
	decl_arr->push_back(new Declaration("a", new BooleanType()));
	::std::vector<Type*>* type_arr = new ::std::vector<Type*>();
	type_arr->push_back(new ArrayType(new IntegerType(8, true), 8));
	type_arr->push_back(new BooleanType());
	type_arr->push_back(new FloatingPointType(8));
	type_arr->push_back(new FunctionType(new BooleanType(), decl_arr));
	type_arr->push_back(new IntegerType(8, true));
	type_arr->push_back(new LabelType());
	type_arr->push_back(new PointerType(new BooleanType()));
	type_arr->push_back(new StructType(StructType::Sequential, "new struct"));
	type_arr->push_back(new VoidType());
	return type_arr;
}

::std::vector<Expression*>* generate_Expression_test_case() {
	::std::vector<Expression*>* exp_arr = new ::std::vector<Expression*>();
	exp_arr->push_back(new AbortExpression(true));
	exp_arr->push_back(new ArraySegmentExpression(new NopExpression(), new NopExpression(), new NopExpression()));
	exp_arr->push_back(new AssignExpression(new NopExpression(), new NopExpression()));
	exp_arr->push_back(new BinaryExpression(Expression::Nop, new NopExpression(), new NopExpression()));
	exp_arr->push_back(new BlockExpression());
	exp_arr->push_back(new CallExpression(new NopExpression()));
	exp_arr->push_back(new CommitExpression(true));
	exp_arr->push_back(new ConditionalExpression());
	exp_arr->push_back(new ConstantExpression(new ConstantValue(false)));
	exp_arr->push_back(new ConvertExpression(new VoidType(), new NopExpression()));
	exp_arr->push_back(new FunctionExpression(new Symbol("new sym", new VoidType()), new SymbolScope(NULL, SymbolScope::Global, new NopExpression())));
	exp_arr->push_back(new GotoExpression("label"));
	exp_arr->push_back(new IndexExpression(new NopExpression(), new NopExpression()));
	exp_arr->push_back(new InlineAssemblyExpression("str", NULL));
	exp_arr->push_back(new LabelExpression(new Symbol("new sym", new VoidType())));
	exp_arr->push_back(new LoopExpression(new NopExpression(), new NopExpression(), new NopExpression(), new NopExpression(), true));
	exp_arr->push_back(new MemberExpression(new NopExpression(), "field"));
	{
		::std::vector<Expression*>* pVec = new ::std::vector<Expression*>();
		pVec->push_back(new NopExpression());
		exp_arr->push_back(new NewRunnerExpression(new NopExpression(),pVec, pVec, pVec, pVec));
	}
	exp_arr->push_back(new NopExpression());
	exp_arr->push_back(new ProgramExpression());
	exp_arr->push_back(new ReturnExpression());
	exp_arr->push_back(new UnaryExpression(Expression::Nop, new NopExpression()));
	exp_arr->push_back(new VariableExpression(new Symbol("new sym", new VoidType())));
	return exp_arr;
}

int main(int argc, char** argv) {
	{
		::std::ofstream _f("xml.xml");
		::std::ofstream _f2("out.bin", ::std::ios::binary);
		::boost::archive::xml_oarchive oa(_f);
		::boost::archive::binary_oarchive oa_bin(_f2);
		::std::vector<Type*>* ret = generate_Type_test_case();
		::std::vector<Expression*>* ret2 = generate_Expression_test_case();
		oa & BOOST_SERIALIZATION_NVP(ret);
		oa & BOOST_SERIALIZATION_NVP(ret2);
		oa_bin & BOOST_SERIALIZATION_NVP(ret);
		oa_bin & BOOST_SERIALIZATION_NVP(ret2);
	}
}*/
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <iterator>
#include <algorithm>

#include <core/Core.h>
#include <frontend/c/CSourceParser.h>
// #include <backend/disa/DisaCodeGenerator.h>
// #include <backend/disa/DisaAssemblyParser.h>
#include <backend/i0/I0CodeGenerator.h>
#include <binary/elf/ElfFileWriter.h>
#include <binary/flat/FlatFileWriter.h>
#include <core/Symbol/SymbolAddressAllocator.h>
#include <core/Pass/ConstantPropagation.h>
#include <core/Pass/TypeDeduction.h>

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

CompilationContext* load_file_context(const char* file)
{
	::std::ifstream f(file);
	::boost::archive::xml_iarchive obj(f);
	CompilationContext* context;
	obj & BOOST_SERIALIZATION_NVP(context);
	return context;
}


int main(int argc, char **argv)
{
	bool is_debugging = false;
	::std::vector<const char*> cc0_obj_files;
	const char* cc0_output_file = NULL;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
            	cc0_output_file = argv[++i];
			}
			else
			{
				std::cerr << "invalid argument!\n";
				return -1;
			}
        }
        else if( (strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-g") == 0) )
        {
            is_debugging = true;
        }
        else
        {
            cc0_obj_files.push_back(argv[i]);
        }
    }

    if(cc0_obj_files.size() == 0)
    {
    	std::cerr << "no obj input applied\n";
    	return 1;
    }
    if(cc0_output_file == NULL)
    {
    	cc0_output_file = "a.bin";
    }

    ::std::vector<CompilationContext*> contexts;
    ::std::transform(cc0_obj_files.begin(), cc0_obj_files.end(), ::std::back_inserter(contexts), load_file_context);


    return 0;

}
