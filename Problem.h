#pragma once
#include <vector>
#include "PaperRoll.h"
#include "Pattern.h"
using namespace std;

class Problem
{
protected:
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