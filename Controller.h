#pragma once
#include "problem.h"
#include "json\json.h"
#include <fstream>
#include <string>
#include "MasterProblem.h"
#include "Subproblem.h"
#include "Utility.h"
using namespace std;

class BPNode
{
public:
	struct PatternBound
	{
		// Signature is a stable textual representation of the pattern content,
		// for example "2,0,1". It is used instead of Pattern::_id so that the
		// same cutting pattern is recognized even if generated as a new object.
		string signature;

		// Bounds apply to the master variable x_pattern, i.e. the number of
		// times this cutting pattern may be used in the current branch node.
		// upperBound == -1 means no additional branch-specific upper bound.
		// The binary master still applies its base upper bound of 1.
		int lowerBound;
		int upperBound;
	};

	BPNode();
	BPNode(int depth);

	void addBound(const string& signature, int lowerBound, int upperBound);
	void setEvaluation(const vector<double>& values, double objective, int patternCount, int sequence);
	bool isEvaluated() const;
	bool isSolvedWithPatternCount(int patternCount) const;

	const vector<PatternBound>& getBounds() const;
	const vector<double>& getValues() const;
	int getId() const;
	double getObjective() const;
	int getDepth() const;
	int getSequence() const;
	void setDepth(int depth);

private:
	// Monotonic identifier for readable branch-and-price logs. Queue copies
	// keep the same id; only newly constructed logical nodes get new ids.
	int _id;
	static int _counter;

	// Branching is implemented by adding bounds on pattern variables.
	// Each child node inherits all parent bounds and adds one more bound.
	vector<PatternBound> _bounds;

	// LP information for best-first search. These fields are filled after the
	// node's restricted master is solved by column generation.
	vector<double> _values;
	double _objective;
	int _patternCount;
	int _sequence;
	int _depth;
	bool _evaluated;
};

class BPNodeCompare
{
public:
	bool operator()(const BPNode& left, const BPNode& right) const;
};

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
	int _bestNodeId;
	int _bestNodeDepth;
	double _lowerBound;
	bool _terminatedByIntegerBound;
	bool _stoppedAtNodeLimit;
	bool _searchTreeExhausted;

	int _processedBranchAndPriceNodes;
	int _maxBranchAndPriceNodes;
	int _nextBranchAndPriceSequence;

	// Global column pool. Every Pattern represents one cutting pattern, and
	// branch-and-price nodes build restricted master problems from this pool.
	vector<Pattern* > _patterns;

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

	// Branch-node bounds are translated into bounds within the base [0, 1]
	// domain when a node's restricted master problem is built.
	bool getPatternBounds(const BPNode& node, Pattern* pattern, double& lowerBound, double& upperBound);

	// Branch-and-price search helpers.
	bool solveColumnGenerationAtNode(const BPNode& node, vector<double>& values, double& objective);
	bool evaluateBPNode(BPNode& node);
	void createChildNodes(const BPNode& node, int branchIndex, BPNode& downNode, BPNode& upNode);
	void solveBranchAndPriceNode(const BPNode& node);
	int findFractionalPatternIndex(const vector<double>& values);
	bool hasBranchAndPriceIncumbent() const;
	bool isIntegerBoundClosed() const;
	void reportBranchAndPriceBounds() const;
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
