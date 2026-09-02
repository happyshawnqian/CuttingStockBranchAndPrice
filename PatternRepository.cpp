#include "PatternRepository.h"
#include "Utility.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

// Item count remains unset until reset() establishes the signature width.
PatternRepository::PatternRepository()
{
	_itemCount = 0;
}

// clear() applies the repository's ownership rule to every stored Pattern.
PatternRepository::~PatternRepository()
{
	clear();
}

// Start a new append-only index space whose signatures contain itemCount bits.
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

// Delete canonical Pattern objects before discarding both lookup structures.
void PatternRepository::clear()
{
	for (auto pattern : _patterns)
	{
		delete pattern;
	}
	_patterns.clear();
	_signatureToRepositoryIndex.clear();
	_itemCount = 0;
}

// Canonicalize by binary signature. New candidates are appended once; duplicate
// candidates are deleted and mapped to the existing stable repository index.
PatternRepository::AddResult PatternRepository::addOrGet(Pattern* candidate)
{
	if (candidate == nullptr)
	{
		cout << "Error, cannot add a null pattern to the repository" << endl;
		exit(1);
	}

	string signature = getSignature(candidate);
	auto known = _signatureToRepositoryIndex.find(signature);
	if (known != _signatureToRepositoryIndex.end())
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
	_signatureToRepositoryIndex[signature] = patternIndex;

	AddResult result = { patternIndex, true };
	return result;
}

// Guard the stable-index boundary before exposing a repository-owned pointer.
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

// Validate binary sparse content and serialize the full item-selection vector,
// making signatures independent of sparse entry order and Pattern reporting ids.
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
