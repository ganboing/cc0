#ifndef REGISTEREXPRESSIONSWITHBOOST_H_
#define REGISTEREXPRESSIONSWITHBOOST_H_

#include "AbortExpression.h"
#include "ArraySegmentExpression.h"
#include "AssignExpression.h"
#include "BinaryExpression.h"
#include "BlockExpression.h"
#include "CallExpression.h"
#include "CommitExpression.h"
#include "ConditionalExpression.h"
#include "ConstantExpression.h"
#include "ConvertExpression.h"
#include "FunctionExpression.h"
#include "GotoExpression.h"
#include "IndexExpression.h"
#include "InlineAssemblyExpression.h"
#include "LabelExpression.h"
#include "LoopExpression.h"
#include "MemberExpression.h"
#include "NewRunnerExpression.h"
#include "NopExpression.h"
#include "ProgramExpression.h"
#include "ReturnExpression.h"
#include "UnaryExpression.h"
#include "VariableExpression.h"

template <class A>
void RegisterExpressionsWithBoost(A& ar)
{
	ar.template register_derived_with_boost<AbortExpression>();
	ar.template register_derived_with_boost<ArraySegmentExpression>();
	ar.template register_derived_with_boost<AssignExpression>();
	ar.template register_derived_with_boost<BinaryExpression>();
	ar.template register_derived_with_boost<BlockExpression>();
	ar.template register_derived_with_boost<CallExpression>();
	ar.template register_derived_with_boost<CommitExpression>();
	ar.template register_derived_with_boost<ConditionalExpression>();
	ar.template register_derived_with_boost<ConstantExpression>();
	ar.template register_derived_with_boost<ConvertExpression>();
	ar.template register_derived_with_boost<FunctionExpression>();
	ar.template register_derived_with_boost<GotoExpression>();
	ar.template register_derived_with_boost<IndexExpression>();
	ar.template register_derived_with_boost<InlineAssemblyExpression>();
	ar.template register_derived_with_boost<LabelExpression>();
	ar.template register_derived_with_boost<LoopExpression>();
	ar.template register_derived_with_boost<MemberExpression>();
	ar.template register_derived_with_boost<NewRunnerExpression>();
	ar.template register_derived_with_boost<NopExpression>();
	ar.template register_derived_with_boost<ProgramExpression>();
	ar.template register_derived_with_boost<ReturnExpression>();
	ar.template register_derived_with_boost<UnaryExpression>();
	ar.template register_derived_with_boost<VariableExpression>();
}

#endif /* REGISTEREXPRESSIONSWITHBOOST_H_ */
