#ifndef REGISTERTYPESWITHBOOST_H_
#define REGISTERTYPESWITHBOOST_H_

#include "Type.h"
#include "ArrayType.h"
#include "BooleanType.h"
#include "FloatingPointType.h"
#include "FunctionType.h"
#include "IntegerType.h"
#include "LabelType.h"
#include "PointerType.h"
#include "StructType.h"
#include "VoidType.h"

template <class A>
void RegisterTypesWithBoost(A& ar)
{
	ar.template register_type<ArrayType>();
	ar.template register_type<BooleanType>();
	ar.template register_type<FloatingPointType>();
	ar.template register_type<FunctionType>();
	ar.template register_type<IntegerType>();
	ar.template register_type<LabelType>();
	ar.template register_type<PointerType>();
	ar.template register_type<StructType>();
	ar.template register_type<VoidType>();
}


#endif /* REGISTERTYPESWITHBOOST_H_ */
