#pragma once

class PaperRoll
{
private:
	int _id;
	int _width;
	int _number;
	bool _isMaterial;	// is material or product, to cut or to be cut?
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

	void setCost(double cost) { _cost = cost; }
	double getCost(){ return _cost; }
};