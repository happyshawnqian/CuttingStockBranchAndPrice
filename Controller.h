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
	// The controller coordinates data loading, column generation, and the
	// branch-and-price search. The Problem object owns the logical instance;
	// the two solver objects are rebuilt whenever the active model must be
	// reset, because CPLEX model objects accumulate columns and constraints.
	Problem* _problem;
	MasterProblem* _masterProblem;
	Subproblem* _subproblem;

	// Ownership flags are needed because a Controller can either create the
	// Problem internally or receive one from outside. Data read from JSON is
	// released by this class; externally supplied data is only referenced.
	bool _ownsProblem;
	bool _ownsMaterials;
	bool _ownsProducts;

	// Incumbent information for branch-and-price. _bestSolution stores the
	// values of the master pattern variables at the best integer leaf found.
	double _bestObjective;
	vector<double> _bestSolution;
	int _processedBranchAndPriceNodes;
	int _maxBranchAndPriceNodes;

	// Global column pool. Every Pattern represents one cutting pattern, and
	// branch-and-price nodes build restricted master problems from this pool.
	vector<Pattern* > _patterns;

	struct PatternBound
	{
		// Signature is a stable textual representation of the pattern content,
		// for example "2,0,1". It is used instead of Pattern::_id so that the
		// same cutting pattern is recognized even if generated as a new object.
		string signature;

		// Bounds apply to the master variable x_pattern, i.e. the number of
		// times this cutting pattern may be used in the current branch node.
		// upperBound == -1 means no finite upper bound.
		int lowerBound;
		int upperBound;
	};

	struct BranchNode
	{
		// Branching is implemented by adding bounds on pattern variables.
		// Each child node inherits all parent bounds and adds one more bound.
		vector<PatternBound> bounds;
		int depth;
	};

	// Clear helpers release only the objects that this controller owns.
	void clearMaterials(bool clearProblemVector);
	void clearProducts(bool clearProblemVector);
	void clearPatterns();

	// Solver helpers keep the raw Problem data synchronized with fresh CPLEX
	// models. Rebuilding is simpler and safer than trying to remove columns.
	void resetSolvers();
	void syncProblemToSolvers();
	void validateProblemReady();

	// Pattern signatures are used for duplicate detection and node branching.
	string getPatternSignature(Pattern* pattern);
	string getPatternSignature(const vector<int>& counts);
	bool isKnownPattern(Pattern* pattern);
	bool isKnownPatternSignature(const string& signature);

	// Branch-node bounds are translated into variable lower/upper bounds when
	// a node's restricted master problem is built.
	bool getPatternBounds(const BranchNode& node, Pattern* pattern, double& lowerBound, double& upperBound);
	void addBranchBound(BranchNode& node, const string& signature, int lowerBound, int upperBound);

	// Pricing for the branch-and-price implementation is enumerative. For the
	// small cutting-stock instances targeted by this project, enumerating all
	// feasible patterns is deterministic and avoids duplicate pricing columns.
	void enumeratePricingPatterns(int productIndex, int remainingWidth, const vector<double>& duals,
		vector<int>& counts, double& bestReducedCost, vector<int>& bestCounts);
	Pattern* findBestPricingPattern(const vector<double>& duals);

	// Branch-and-price search helpers.
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

	void solveCG();	// Solve the LP relaxation by column generation only.
	void solveIP();	// Solve an integer restricted master using generated columns.
	void solveBP();	// Solve with branch-and-price over pattern variables.
	vector<Pattern* > findInitialPatterns(); // Generate one simple pattern per product.

};
