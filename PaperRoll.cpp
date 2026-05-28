#include "PaperRoll.h"

PaperRoll::PaperRoll(int width, int number, bool isMaterial) : _width(width), _number(number), _isMaterial(isMaterial)
{
	_cost = 1; // equal 1 by default
	_id = _counter;
	_counter++;
}

int PaperRoll::_counter = 0;