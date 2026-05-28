#include "Pattern.h"

int Pattern::_counter = 0;

Pattern::Pattern()
{
	_id = _counter;
	_counter++;
	_cost = 1;	// by default cost is 1
}

void Pattern::print()
{
	cout << endl;
	cout << "Pattern " << _id << endl;
	for (auto content : _content)
	{
		cout << "  produce Product " << content.first << ", " << content.second << endl;
	}
	cout << endl;
}