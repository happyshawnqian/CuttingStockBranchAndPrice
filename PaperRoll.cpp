#include "PaperRoll.h"

PaperRoll::PaperRoll(int width, int number, bool isMaterial) : _width(width), _number(number), _isMaterial(isMaterial)
{
	// Materials default to unit roll cost. Products retain the same field for
	// the shared data representation, although their cost is not used.
	_cost = 1;
	_sourceProductIndex = -1;
	_copyIndex = -1;
	_id = _counter;
	_counter++;
}

int PaperRoll::_counter = 0;
