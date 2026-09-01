#pragma once

#include "Pattern.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// Owns every unique cutting pattern generated during one solve. Repository
// indices are append-only and remain stable until clear() or reset() is called.
class PatternRepository
{
public:
	struct AddResult
	{
		int patternIndex;
		bool inserted;
	};

	PatternRepository();
	~PatternRepository();

	PatternRepository(const PatternRepository&) = delete;
	PatternRepository& operator=(const PatternRepository&) = delete;

	void reset(int itemCount);
	void clear();

	// Takes ownership of candidate in both the inserted and duplicate cases.
	AddResult addOrGet(Pattern* candidate);
	Pattern* getPattern(int patternIndex) const;
	int size() const;

private:
	int _itemCount;
	vector<Pattern* > _patterns;
	unordered_map<string, int> _signatureToRepositoryIndex;

	string getSignature(Pattern* pattern) const;
};
