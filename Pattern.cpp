#include "Pattern.h"

int Pattern::_counter = 0;

Pattern::Pattern()
{
	// Pattern IDs are assigned sequentially for readable output and CPLEX names.
	// Equality between patterns is determined elsewhere by product counts.
	_id = _counter;
	_counter++;
	_cost = 1;	// by default cost is 1
}

void Pattern::print()
{
	// Print the sparse product-count representation of this cutting pattern.
	cout << endl;
	cout << "Pattern " << _id << endl;
	for (auto content : _content)
	{
		cout << "  produce Product " << content.first << ", " << content.second << endl;
	}
	cout << endl;
}
