#include <fstream>
#include <vector>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>

::std::vector<Type*>* generate_Type_test_case() {
	::std::vector<Declaration*>* decl_arr = new ::std::vector<Declaration*>();
	decl_arr->push_back(new Declaration("a", new BooleanType()));
	::std::vector<Type*> type_arr;
	type_arr.push_back(new ArrayType(new IntegerType(8, true), 8));
	type_arr.push_back(new BooleanType());
	type_arr.push_back(new FloatingPointType(8));
	type_arr.push_back(new FunctionType(new BooleanType(), decl_arr));
	type_arr.push_back(new IntegerType(8, true));
	type_arr.push_back(new LabelType());
	type_arr.push_back(new PointerType(new BooleanType()));
	type_arr.push_back(new StructType(StructType::Sequential, "new struct"));
	type_arr.push_back(new VoidType());
	return decl_arr;
}

int main(int argc, char** argv) {
	{
		::std::ofstream _f("xml.xml");
		::boost::archive::xml_oarchive oa(_f);
		oa & BOOST_SERIALIZATION_NVP(generate_Type_test_case());
	}
}
