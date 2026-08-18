#include "PatternRepository.h"
#include "Utility.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

PatternRepository::PatternRepository()
{
	_itemCount = 0;
}

PatternRepository::~PatternRepository()
{
	clear();
}

void PatternRepository::reset(int itemCount)
{
	if (itemCount <= 0)
	{
		cout << "Error, pattern repository requires a positive item count" << endl;
		exit(1);
	}

	clear();
	_itemCount = itemCount;
}

void PatternRepository::clear()
{
	for (auto pattern : _patterns)
	{
		delete pattern;
	}
	_patterns.clear();
	_indexBySignature.clear();
	_itemCount = 0;
}

PatternRepository::AddResult PatternRepository::addOrGet(Pattern* candidate)
{
	if (candidate == nullptr)
	{
		cout << "Error, cannot add a null pattern to the repository" << endl;
		exit(1);
	}

	string signature = getSignature(candidate);
	auto known = _indexBySignature.find(signature);
	if (known != _indexBySignature.end())
	{
		int knownIndex = known->second;
		Pattern* knownPattern = _patterns[knownIndex];
		if (fabs(knownPattern->getCost() - candidate->getCost()) > Utility::RC_EPS)
		{
			cout << "Error, duplicate pattern signatures have different costs" << endl;
			delete candidate;
			exit(1);
		}

		delete candidate;
		AddResult result = { knownIndex, false };
		return result;
	}

	int patternIndex = static_cast<int>(_patterns.size());
	_patterns.push_back(candidate);
	_indexBySignature[signature] = patternIndex;

	AddResult result = { patternIndex, true };
	return result;
}

Pattern* PatternRepository::getPattern(int patternIndex) const
{
	if (patternIndex < 0 || patternIndex >= static_cast<int>(_patterns.size()))
	{
		cout << "Error, invalid global pattern repository index "
			<< patternIndex << endl;
		exit(1);
	}
	return _patterns[patternIndex];
}

int PatternRepository::size() const
{
	return static_cast<int>(_patterns.size());
}

string PatternRepository::getSignature(Pattern* pattern) const
{
	if (_itemCount <= 0)
	{
		cout << "Error, pattern repository is not initialized" << endl;
		exit(1);
	}

	vector<int> selected(_itemCount, 0);
	for (auto content : pattern->getContent())
	{
		if (content.first < 0 || content.first >= _itemCount
			|| content.second != 1 || selected[content.first] != 0)
		{
			cout << "Error, repository patterns must be non-duplicated binary subsets"
				<< endl;
			exit(1);
		}
		selected[content.first] = 1;
	}

	bool isEmpty = true;
	ostringstream signature;
	for (int itemIndex = 0; itemIndex < _itemCount; itemIndex++)
	{
		if (itemIndex > 0)
		{
			signature << ',';
		}
		signature << selected[itemIndex];
		if (selected[itemIndex] == 1)
		{
			isEmpty = false;
		}
	}

	if (isEmpty)
	{
		cout << "Error, an empty pattern cannot be stored in the repository" << endl;
		exit(1);
	}
	return signature.str();
}
