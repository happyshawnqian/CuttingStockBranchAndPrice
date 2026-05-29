#pragma once
#include "problem.h"
#include "json\json.h"
#include <fstream>
#include <string>
#include "MasterProblem.h"
#include "Subproblem.h"
using namespace std;

class Controller
{
private:
	Problem* _problem;
	MasterProblem* _masterProblem;
	Subproblem* _subproblem;
	bool _ownsProblem;
	bool _ownsMaterials;
	bool _ownsProducts;
	double _bestObjective;
	vector<double> _bestSolution;
	int _processedBranchAndPriceNodes;
	int _maxBranchAndPriceNodes;

	vector<Pattern* > _patterns;

	struct PatternBound
	{
		string signature;
		int lowerBound;
		int upperBound;
	};

	struct BranchNode
	{
		vector<PatternBound> bounds;
		int depth;
	};

	void clearMaterials(bool clearProblemVector);
	void clearProducts(bool clearProblemVector);
	void clearPatterns();
	void resetSolvers();
	void syncProblemToSolvers();
	void validateProblemReady();
	string getPatternSignature(Pattern* pattern);
	string getPatternSignature(const vector<int>& counts);
	bool isKnownPattern(Pattern* pattern);
	bool isKnownPatternSignature(const string& signature);
	bool getPatternBounds(const BranchNode& node, Pattern* pattern, double& lowerBound, double& upperBound);
	void addBranchBound(BranchNode& node, const string& signature, int lowerBound, int upperBound);
	void enumeratePricingPatterns(int productIndex, int remainingWidth, const vector<double>& duals,
		vector<int>& counts, double& bestReducedCost, vector<int>& bestCounts);
	Pattern* findBestPricingPattern(const vector<double>& duals);
	bool solveColumnGenerationAtNode(const BranchNode& node, vector<double>& values, double& objective);
	void solveBranchAndPriceNode(const BranchNode& node);
	int findFractionalPatternIndex(const vector<double>& values);
	void reportBranchAndPriceSolution();

public:
	Controller();
	Controller(Problem* problem);
	~Controller();

	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

	void setProblem(Problem* problem);
	Problem* getProblem() { return _problem; }

	void loadMaterials(string dataFile);
	void loadProducts(string dataFile);

	void solveCG();	// solve by column generation
	void solveIP();	// solve as an IP by generated columns
	void solveBP();	// solve by branch and price
	vector<Pattern* > findInitialPatterns(); // generate initial patterns

};
