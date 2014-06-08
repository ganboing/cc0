#include <fstream>
#include <iostream>
#include <vector>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

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
}
