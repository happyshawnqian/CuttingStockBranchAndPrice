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
		// Stable position of the canonical pattern in the append-only repository.
		int patternIndex;
		// True when the candidate was new; false when an equivalent pattern existed.
		bool inserted;
	};

	// Create an empty, uninitialized repository.
	PatternRepository();
	// Delete every pattern owned by the repository.
	~PatternRepository();

	PatternRepository(const PatternRepository&) = delete;
	PatternRepository& operator=(const PatternRepository&) = delete;

	// Delete existing patterns and initialize signatures for itemCount products.
	void reset(int itemCount);
	// Delete all owned patterns and return the repository to its uninitialized state.
	void clear();

	// Return the canonical repository index for candidate. The repository takes
	// ownership in both the inserted and duplicate cases and rejects cost conflicts.
	AddResult addOrGet(Pattern* candidate);
	// Return the pattern at a stable global repository index or terminate if invalid.
	Pattern* getPattern(int patternIndex) const;
	int size() const;

private:
	int _itemCount;
	vector<Pattern* > _patterns;
	unordered_map<string, int> _signatureToRepositoryIndex;

	// Encode a nonempty binary pattern as a deterministic item-selection string.
	string getSignature(Pattern* pattern) const;
};
