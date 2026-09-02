#pragma once
#include <vector>
#include "PaperRoll.h"
#include "Pattern.h"
using namespace std;

class Problem
{
protected:
	// Materials describe raw-roll types. Products are distinct unit-demand item
	// copies; JSON loading creates them by expanding each product demand. The
	// vectors store pointers, while Controller handles their ownership.
	vector<PaperRoll* > _materials;
	vector<PaperRoll* > _products;
public:
	// Create an empty, non-owning view of materials and products.
	Problem(){}
// Destroy the view without deleting rolls owned by Controller or an external caller.
	virtual ~Problem() {}
	// Create a non-owning view over the supplied material and product vectors.
	Problem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products) :_materials(materials), _products(products)
	{

	}

	void setMaterials(const vector<PaperRoll* >& materials) { _materials = materials; }
	vector<PaperRoll* > getMaterials(){ return _materials; }

	void setProducts(const vector<PaperRoll* >& products) { _products = products; }
	vector<PaperRoll* > getProducts(){ return _products; }
};
