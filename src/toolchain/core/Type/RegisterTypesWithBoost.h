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
	ar.template register_derived_with_boost<ArrayType>();
	ar.template register_derived_with_boost<BooleanType>();
	ar.template register_derived_with_boost<FloatingPointType>();
	ar.template register_derived_with_boost<FunctionType>();
	ar.template register_derived_with_boost<IntegerType>();
	ar.template register_derived_with_boost<LabelType>();
	ar.template register_derived_with_boost<PointerType>();
	ar.template register_derived_with_boost<StructType>();
	ar.template register_derived_with_boost<VoidType>();
}


#endif /* REGISTERTYPESWITHBOOST_H_ */
