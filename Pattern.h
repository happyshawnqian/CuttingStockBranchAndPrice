#pragma once
#include <vector>
#include <iostream>
using namespace std;

class Pattern
{
private:
	int _id;
	vector<pair<int, int>> _content;	// a pair is <product_id, number>, how many contained in the pattern for product id 
	double _cost;
	static int _counter;
public:
	Pattern();

	void setId(int id){ _id = id; }
	int getId(){ return _id; }

	void setContent(vector<pair<int, int>> content){ _content = content; }
	vector<pair<int, int>> getContent(){ return _content; }
	void addContent(pair<int, int> content) { _content.push_back(content); }

	void setCost(double cost) { _cost = cost; }
	double getCost(){ return _cost; }

	void print();
};