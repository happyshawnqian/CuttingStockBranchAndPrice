#pragma once

class PaperRoll
{
private:
	int _id;

	// Width is the physical roll width. For unit-demand products, _number is 1;
	// for materials, it is the available capacity read from JSON.
	int _width;
	int _number;
	bool _isMaterial;	// true for raw material, false for demanded product

	// Unit-demand products keep their origin in products.json. Materials and
	// products not created by the JSON loader use -1 for both fields.
	int _sourceProductIndex;
	int _copyIndex;

	// Cost is used by the master/pricing objective. The current data loader
	// keeps it at the default value 1 unless it is set manually.
	double _cost;
	static int _counter;

public:
	PaperRoll(int width, int number, bool isMaterial);

	void setId(int id) { _id = id; }
	int getId(){ return _id; }

	void setWidth(int width) { _width = width; }
	int getWidth(){ return _width; }

	void setNumber(int number) { _number = number; }
	int getNumber(){ return _number; }

	void setIsMaterial(bool isMaterial) { _isMaterial = isMaterial; }
	bool getIsMaterial(){ return _isMaterial; }

	void setSourceProductIndex(int sourceProductIndex) { _sourceProductIndex = sourceProductIndex; }
	int getSourceProductIndex() { return _sourceProductIndex; }

	void setCopyIndex(int copyIndex) { _copyIndex = copyIndex; }
	int getCopyIndex() { return _copyIndex; }

	void setCost(double cost) { _cost = cost; }
	double getCost(){ return _cost; }
};
