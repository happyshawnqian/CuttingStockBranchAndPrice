#pragma once
#include <vector>
#include "PaperRoll.h"
#include "Pattern.h"
using namespace std;

class Problem
{
protected:
	// Materials are raw rolls; products are demanded roll widths. The vectors
	// store pointers for compatibility with the original project structure.
	// Ownership is intentionally handled by Controller, not by this base class.
	vector<PaperRoll* > _materials;
	vector<PaperRoll* > _products;
public:
	Problem(){}
	virtual ~Problem() {}
	Problem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products) :_materials(materials), _products(products)
	{

	}

	void setMaterials(const vector<PaperRoll* >& materials) { _materials = materials; }
	vector<PaperRoll* > getMaterials(){ return _materials; }

	void setProducts(const vector<PaperRoll* >& products) { _products = products; }
	vector<PaperRoll* > getProducts(){ return _products; }
};
