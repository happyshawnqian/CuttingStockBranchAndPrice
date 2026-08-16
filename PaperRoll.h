#pragma once

class PaperRoll
{
private:
	int _id;

	// _width is the physical width of a material or product item. _number stores
	// material capacity and is 1 for every expanded product item; JSON loading
	// initializes both values from the corresponding input fields.
	int _width;
	int _number;
	bool _isMaterial;	// true for raw material, false for demanded product

	// Unit-demand products keep their origin in products.json. Materials and
	// products not created by the JSON loader use -1 for both fields.
	int _sourceProductIndex;
	int _copyIndex;

	// Raw-material cost is used by pricing and copied to generated patterns.
	// Product cost is unused; the current data loader leaves both at 1.
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
