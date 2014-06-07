#pragma once 

#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

class ILClass;
class SymbolScope;

class ILProgram
{
private:
	friend ::boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int ver)
	{
		ar & Claases;
		ar & Scope;
	}
public:
    std::vector<ILClass *> Claases;
    SymbolScope *Scope;
    ILProgram();
    virtual ~ILProgram();
};

