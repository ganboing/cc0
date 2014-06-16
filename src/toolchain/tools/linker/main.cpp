#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <iterator>
#include <algorithm>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/Core.h>
#include <frontend/c/CSourceParser.h>
// #include <backend/disa/DisaCodeGenerator.h>
// #include <backend/disa/DisaAssemblyParser.h>
#include <backend/i0/I0CodeGenerator.h>
#include <binary/elf/ElfFileWriter.h>
#include <binary/flat/FlatFileWriter.h>
#include <core/Symbol/SymbolAddressAllocator.h>
#include "core/Symbol/SymbolScope.h"
#include <core/Pass/ConstantPropagation.h>
#include <core/Pass/TypeDeduction.h>
#include "core/Serialization/ExportDeriveTypes.h"
#include "core/Serialization/ExportDeriveExpressions.h"
#include "core/Serialization/ObjFormat.h"

#include "../../../external/mem.h"
#include "../../../external/sys_config.h"

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

namespace {
template<class Container>
class TagIterator: public ::std::pair<typename Container::const_iterator,
		const Container*> {
private:
	TagIterator();
public:
	typedef ::std::pair<typename Container::const_iterator, const Container*> _parent_type;
	typedef typename _parent_type::first_type first_type;
	typedef typename _parent_type::second_type second_type;
	TagIterator(const first_type& a, const second_type& b) :
			_parent_type(a, b) {
	}
	TagIterator(const second_type& container) :
			_parent_type(container->begin(), container) {
	}
	TagIterator(const TagIterator& other) :
			_parent_type(other) {
	}
	bool operator<(const TagIterator& other) const {
		return *(*this).first < *other.first;
	}
	bool operator>(const TagIterator& other) const {
		return *other.first < *(*this).first;
	}
};

template<class Container>
class TagItPool: public ::std::priority_queue<TagIterator<Container>,
		::std::vector<TagIterator<Container> >,
		::std::greater<TagIterator<Container> > > {
public:
	typedef ::std::priority_queue<TagIterator<Container>,
			::std::vector<TagIterator<Container> >,
			::std::greater<TagIterator<Container> > > _parent_type;
	typedef typename _parent_type::value_type value_type;
	typedef typename _parent_type::size_type size_type;
	void push(const value_type& tagit) {
		if (tagit.first != tagit.second->end()) {
			_parent_type::push(tagit);
		}
	}
	value_type pull() {
		value_type ret = _parent_type::top();
		_parent_type::pop();
		value_type nxt(ret);
		++nxt.first;
		push(nxt);
		return ret;
	}
	::std::list<value_type> next() {
		::std::list<value_type> ret;
		ret.push_back(pull());
		while (!_parent_type::empty()) {
			if (_parent_type::top().first->first == ret.front().first->first) {
				ret.push_back(pull());
			} else {
				break;
			}
		}
		return ret;
	}
};

typedef ::std::map< ::std::string, Symbol*> symbol_map_t;
typedef TagIterator<symbol_map_t> sym_tagit_t;
typedef TagItPool<symbol_map_t> sym_tagit_pool_t;
typedef ::std::set< ::std::string> realloc_record_t;
typedef ::std::map<SymbolScope*, realloc_record_t> realloc_map_t;
typedef ::std::list<sym_tagit_t> sym_merge_list_t;
typedef ::std::set<SymbolScope*> scope_set_t;
typedef ::std::vector<SymbolScope*> scope_vec_t;

}

void DumpScope(SymbolScope *scope, std::ofstream &dump) {
	char buffer[100];

	for (std::map<std::string, Symbol *>::iterator it =
			scope->GetSymbolTable()->begin();
			it != scope->GetSymbolTable()->end(); ++it) {
		Symbol *symbol = it->second;
		if (typeid(*(symbol->DeclType)) == typeid(FunctionType)
				|| scope->GetScopeKind() == SymbolScope::Global) {
			sprintf(buffer, "%0llX\t%s", (long long) symbol->Address,
					symbol->Name.c_str());
			dump << buffer << std::endl;
		}
	}

	for (std::vector<SymbolScope *>::iterator it =
			scope->GetChildScopes()->begin();
			it != scope->GetChildScopes()->end(); ++it) {
		SymbolScope *cs = *it;
		DumpScope(cs, dump);
	}
}

CC0Obj load_file_context(const char* file) {
	::std::ifstream f(file);
	::boost::archive::xml_iarchive obj_file(f);
	CC0Obj obj;
	obj_file & BOOST_SERIALIZATION_NVP(obj);
	return obj;
}

bool check_merge_symbol(const ::std::list<sym_tagit_t>& list) {

	Symbol* pFirstSym = list.front().first->second;
	Type* pFirstTy = pFirstSym->DeclType;
	Type::TypeSpecifier FirstSpec = pFirstTy->GetSpecifiers();
	if (dynamic_cast<FunctionType*>(pFirstTy)) {
		return true;
	}
	for (::std::list<sym_tagit_t>::const_iterator i = list.begin(), iE =
			list.end(); i != iE; ++i) {
		Symbol* pSym = i->first->second;
		if (pSym->Kind != Symbol::ObjectName) {
			::std::cerr << pSym->Name << " symbol kind disagree!\n";
			return false;
		}
		Type* pTy = pSym->DeclType;
		if (!pFirstTy->Equals(pTy)) {
			::std::cerr << pSym->Name << " symbol type disagree!\n";
			return false;
		}
		Type::TypeSpecifier Spec = pTy->GetSpecifiers();
		if(FirstSpec != Spec)
		{
			::std::cerr << pSym->Name << " symbol type specifiers disagree!\n";
			return false;
		}
	}
	return true;
}

void fix_ilprogram_il_symref(ILProgram* ilprogram, SymbolScope* new_global) {
	SymbolScope* old_global = ilprogram->Scope;
	for (::std::vector<ILClass*>::iterator i = ilprogram->Claases.begin(), iE =
			ilprogram->Claases.end(); i != iE; ++i) {
		for (::std::vector<ILFunction *>::iterator j = (*i)->Functions.begin(),
				jE = (*i)->Functions.end(); j != jE; ++j) {
			assert((*j)->FunctionSymbol->Scope == old_global);
			(*j)->FunctionSymbol->Scope = new_global;
			for (::std::vector<IL>::iterator k = (*j)->Body.begin(), kE =
					(*j)->Body.end(); k != kE; ++k) {
				for (::std::vector<IL::ILOperand>::iterator p =
						(*k).Operands.begin(), pE = (*k).Operands.end();
						p != pE; ++p) {
					if ((*p).OperandKind == IL::Variable) {
						if ((*p).SymRef->Scope == old_global) {
							(*p).SymRef->Scope = new_global;
						}
					}
				}
			}
		}
	}
}

ILProgram* merge(::std::vector<ILProgram*> ilprograms) {
	realloc_map_t realloc_map;
	sym_tagit_pool_t pool;
	scope_set_t global_scope_set;
	SymbolScope* new_global_scope = new SymbolScope(NULL, SymbolScope::Global,
	NULL);
	scope_vec_t* new_sub_scopes = new_global_scope->GetChildScopes();
	for (::std::vector<ILProgram*>::iterator i = ilprograms.begin(), iE =
			ilprograms.end(); i != iE; ++i) {
		fix_ilprogram_il_symref(*i, new_global_scope);
	}
	for (scope_vec_t::iterator i = new_sub_scopes->begin(), iE =
			new_sub_scopes->end(); i != iE; ++i) {
		(*i)->_parentScope = new_global_scope;
	}
	::std::vector<ILFunction*> new_global_functions;
	for (::std::vector<ILProgram*>::iterator i = ilprograms.begin(), iE =
			ilprograms.end(); i != iE; ++i) {
		SymbolScope* sym_scope = (*i)->Scope;
		global_scope_set.insert(sym_scope);
		symbol_map_t& sym_tab = sym_scope->_symbolTable;
		pool.push(sym_tagit_t(&sym_tab));
		new_sub_scopes->insert(new_sub_scopes->end(),
				sym_scope->GetChildScopes()->begin(),
				sym_scope->GetChildScopes()->end());
		for (::std::vector<ILClass*>::iterator j = (*i)->Claases.begin(), jE =
				(*i)->Claases.end(); j != jE; ++j) {
			new_global_functions.insert(new_global_functions.end(),
					(*j)->Functions.begin(), (*j)->Functions.end());
		}
	}
	if (CompilationContext::GetInstance()->Debug) {
		::std::cout << "----------\n";
	}
	while (!pool.empty()) {
		sym_merge_list_t ret(pool.next());
		for (sym_merge_list_t::iterator i = ret.begin(), iE = ret.end();
				i != iE; ++i) {
			/*const symbol_map_t* map_in_scope = i->second;
			 const ::std::string& symbol_in_tab = i->first->first;
			 realloc_map[container_of(map_in_scope, SymbolScope, _symbolTable)].insert(symbol_in_tab);*/
			if (CompilationContext::GetInstance()->Debug) {
				std::cout << i->first->first << " with symbol == "
						<< i->first->second << " on scope " << i->second
						<< "\n";
			}
		}
		if (CompilationContext::GetInstance()->Debug) {
			::std::cout << "----------\n";
		}
		if (!check_merge_symbol(ret)) {
			exit(-1);
		}
		new_global_scope->Add(ret.front().first->second);
	}
	if (CompilationContext::GetInstance()->Debug) {
		::std::cout << "new global symbols:\n";
		symbol_map_t* sym_map = new_global_scope->GetSymbolTable();
		{
			symbol_map_t::iterator i = sym_map->begin(), iE = sym_map->end();
			while (1) {
				const ::std::string sym = i->first;
				++i;
				if (i != iE) {
					::std::cout << "├─" << sym << "\n";
				} else {
					::std::cout << "└─" << sym << "\n";
					break;
				}
			}
		}
	}
	ILProgram* new_ilprogram = new ILProgram();
	new_ilprogram->Scope = new_global_scope;
	ILClass* new_ilclass = new ILClass(new_ilprogram,
			new Symbol("Program", new VoidType()));
	new_ilprogram->Claases.push_back(new_ilclass);
	new_ilclass->Functions = new_global_functions;
	return new_ilprogram;
}

int main(int argc, char **argv) {
	CompilationContext *context = CompilationContext::GetInstance();

// context->TextStart =  0x400000000;
// context->DataStart =  0x400004000;
// context->RDataStart = 0x400008000;
// Use macros from the sys_config.h
	context->TextStart = I0_CODE_BEGIN;
	context->DataStart = I0_CODE_BEGIN + 0x4000;
	context->RDataStart = I0_CODE_BEGIN + 0x8000;

//NOTE: Currently, all global variables are put in the bss section and are NOT initialized with zeros, the data/rdata is not used.
// context->BssStart =   0x440000000;
	context->BssStart = AMR_OFFSET_BEGIN;

// NOTE: default targe code type
// Only CODE_TYPE_I0 is supported
	CompilationContext::GetInstance()->CodeType = CODE_TYPE_I0;
	::std::vector<const char*> cc0_obj_files;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
			if (argv[i + 1] != NULL && *argv[i + 1] != '-') {
				CompilationContext::GetInstance()->OutputFile = argv[++i];
			} else {
				std::cerr << "invalid argument!\n";
				return -1;
			}
		} else if ((strcmp(argv[i], "--debug") == 0)
				|| (strcmp(argv[i], "-g") == 0)) {
			CompilationContext::GetInstance()->Debug = true;
		} else {
			cc0_obj_files.push_back(argv[i]);
		}
	}

	if (cc0_obj_files.size() == 0) {
		std::cerr << "no obj input applied\n";
		return 1;
	}
	if (!CompilationContext::GetInstance()->OutputFile.size()) {
		CompilationContext::GetInstance()->OutputFile = "a.bin";
	}

	::std::vector<CC0Obj> objfiles;
	::std::transform(cc0_obj_files.begin(), cc0_obj_files.end(),
			::std::back_inserter(objfiles), load_file_context);
	::std::vector<ILProgram*> ilprograms;
	for (::std::vector<CC0Obj>::iterator i = objfiles.begin(), iE =
			objfiles.end(); i != iE; ++i) {
		ilprograms.push_back(i->second);
	}
	ILProgram* new_il = merge(ilprograms);

	context->IL = new_il;
	SymbolScope::__SetRootScopt(new_il);

	if (CompilationContext::GetInstance()->Debug) {
		std::ofstream ildump("debug.ildump");
		for (::std::vector<ILClass *>::iterator cit = new_il->Claases.begin(),
				citE = new_il->Claases.end(); cit != citE; ++cit) {
			ILClass *c = *cit;

			ildump << "class " << c->ClassSymbol->Name << std::endl << "{"
					<< std::endl;

			for (std::vector<ILFunction *>::iterator fit = c->Functions.begin();
					fit != c->Functions.end(); ++fit) {
				ILFunction *f = *fit;
				ildump << "    function " << f->FunctionSymbol->Name
						<< std::endl << "    {" << std::endl;
				for (std::vector<IL>::iterator iit = f->Body.begin();
						iit != f->Body.end(); ++iit) {
					IL &il = *iit;
					if (il.Opcode == IL::Label) {
						ildump << "        " << il.ToString() << std::endl;
					} else {
						ildump << "            " << il.ToString() << std::endl;
					}
				}
				ildump << "    }" << std::endl;
			}
			ildump << "}" << std::endl;
		}

		ildump.close();
	}
	CodeGenerator* codegen = new I0CodeGenerator();
	codegen->Generate(context->IL);

	if (CompilationContext::GetInstance()->Debug) {
		std::string dumpFileName, mapFileName;
		dumpFileName = "debug.objdump";
		mapFileName = "debug.map";

		std::ofstream objdump(dumpFileName.c_str());
		int64_t currentText = context->TextStart;
		for (std::vector<TargetInstruction *>::iterator iit =
				context->Target->Code.begin();
				iit != context->Target->Code.end(); ++iit) {
			TargetInstruction *inst = *iit;
			char buffer[32];
			sprintf(buffer, "%0llX> \t", (long long) currentText);
			objdump << buffer << inst->ToString().c_str() << std::endl;
			currentText += inst->GetLength();
		}

		std::ofstream mapdump(mapFileName.c_str());
		DumpScope(SymbolScope::GetRootScope(), mapdump);
	}
	printf("Maximum stack frame size: 0x%llX\n",
			(long long) (context->MaxStackFrame));

	char *textBuf = new char[0x100000];
	int64_t textSize = 0;
	for (std::vector<TargetInstruction *>::iterator it =
			context->Target->Code.begin(); it != context->Target->Code.end();
			++it) {
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
