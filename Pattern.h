#pragma once
#include <vector>
#include <iostream>
using namespace std;

class Pattern
{
private:
	int _id;

	// Each entry is <product_index, 1>. Products are distinct unit-demand items,
	// so a binary pattern either contains an item once or does not contain it.
	vector<pair<int, int>> _content;

	// Cost of using this pattern once. In the standard one-stock cutting-stock
	// case this is 1, because one pattern consumes one raw roll.
	double _cost;

	// Simple monotonic identifier used for reporting and CPLEX variable names.
	static int _counter;
public:
	// Create an empty unit-cost pattern with a new reporting identifier.
	Pattern();

	void setId(int id){ _id = id; }
	int getId(){ return _id; }

	void setContent(vector<pair<int, int>> content){ _content = content; }
	vector<pair<int, int>> getContent(){ return _content; }
	// Append one product-index/count entry to the sparse pattern representation.
	void addContent(pair<int, int> content) { _content.push_back(content); }

	void setCost(double cost) { _cost = cost; }
	double getCost(){ return _cost; }

	// Print the pattern identifier and every sparse product-count entry.
	void print();
};
