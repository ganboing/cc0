#include "core/Symbol/Declaration.h"
#include "core/Type/ArrayType.h"
#include "core/Type/BooleanType.h"
#include "core/Type/FloatingPointType.h"
#include "core/Type/FunctionType.h"
#include "core/Type/IntegerType.h"
#include "core/Type/LabelType.h"
#include "core/Type/PointerType.h"
#include "core/Type/StructType.h"
#include "core/Type/VoidType.h"
#include <fstream>
#include <vector>
#include "core/Serialization/ExportDeriveTypes.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>

int main(int argc, char** argv)
{
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
	{
		::std::ofstream _f("xml.xml");
		::boost::archive::xml_oarchive oa(_f);
		oa & BOOST_SERIALIZATION_NVP(type_arr);
	}
}
