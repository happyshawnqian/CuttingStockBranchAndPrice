#include "PaperRoll.h"

PaperRoll::PaperRoll(int width, int number, bool isMaterial) : _width(width), _number(number), _isMaterial(isMaterial)
{
	// The default model charges one unit of cost per raw roll. Products keep
	// the same field for consistency, although only material cost is used.
	_cost = 1; // equal 1 by default
	_sourceProductIndex = -1;
	_copyIndex = -1;
	_id = _counter;
	_counter++;
}

int PaperRoll::_counter = 0;
