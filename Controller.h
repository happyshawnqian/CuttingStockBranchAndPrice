#pragma once
#include "problem.h"
#include "json\json.h"
#include <fstream>
#include <string>
#include "MasterProblem.h"
#include "PatternRepository.h"
#include "Subproblem.h"
#include "Utility.h"
#include <unordered_set>
using namespace std;

class BPNode
{
public:
	// A branch decision restricts every pattern available below the node.
	// TOGETHER permits patterns containing both items or neither item, while
	// SEPARATE forbids patterns containing both items at the same time.
	enum class RyanFosterBranchType
	{
		TOGETHER,
		SEPARATE
	};

	struct RyanFosterConstraint
	{
		// Product indices identify distinct unit-demand items. The first index is
		// always smaller so the same pair has one canonical representation. Each
		// node stores every pair decision inherited along its root-to-node path.
		int firstItemIndex;
		int secondItemIndex;
		RyanFosterBranchType branchType;
	};

	BPNode();
	BPNode(int depth);

	void addRyanFosterConstraint(int firstItemIndex, int secondItemIndex,
		RyanFosterBranchType branchType);
	void setActivePatternIndices(const vector<int>& activePatternIndices);
	void setExactEvaluation(const vector<int>& activePatternIndices,
		const vector<double>& values, double objective, int sequence,
		int poolColumnCount, int exactColumnCount, int exactSolveCount);

	const vector<RyanFosterConstraint>& getRyanFosterConstraints() const;
	const vector<int>& getActivePatternIndices() const;
	const vector<double>& getValues() const;
	int getId() const;
	double getObjective() const;
	int getDepth() const;
	int getSequence() const;
	int getPoolColumnCount() const;
	int getExactColumnCount() const;
	int getExactSolveCount() const;
	bool hasExactPricingCertificate() const;
	void setDepth(int depth);

private:
	// Monotonic identifier for readable branch-and-price logs. Queue copies
	// keep the same id; only newly constructed logical nodes get new ids.
	int _id;
	static int _counter;

	// Each child inherits all parent pair decisions and adds one new decision.
	vector<RyanFosterConstraint> _ryanFosterConstraints;

	// Global repository indices define the node-specific RMP columns. _values
	// uses the same local order after the node has completed exact pricing.
	vector<int> _activePatternIndices;
	vector<double> _values;
	double _objective;
	int _sequence;
	int _depth;
	int _poolColumnCount;
	int _exactColumnCount;
	int _exactSolveCount;
	bool _hasExactPricingCertificate;
};

class BPNodeCompare
{
public:
	bool operator()(const BPNode& left, const BPNode& right) const;
};

class Controller
{
private:
	struct RyanFosterPair
	{
		// togetherValue is y_ij = sum { x_p : pattern p contains i and j }.
		// A value strictly between zero and one identifies a valid branch pair.
		int firstItemIndex;
		int secondItemIndex;
		double togetherValue;
	};

	struct PatternUsage
	{
		int repositoryIndex;
		int quantity;
	};

	// The controller coordinates data loading, column generation, and the
	// branch-and-price search. The Problem object stores the logical instance;
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

	// Incumbent entries use stable repository indices and therefore remain valid
	// when later nodes append more patterns to the global repository.
	double _bestObjective;
	vector<PatternUsage> _bestSolution;
	int _bestNodeId;
	int _bestNodeDepth;
	double _lowerBound;
	bool _terminatedByIntegerBound;
	bool _stoppedAtNodeLimit;
	bool _searchTreeExhausted;

	int _processedBranchAndPriceNodes;
	int _maxBranchAndPriceNodes;
	int _nextBranchAndPriceSequence;

	// Global, append-only repository shared by all branch-and-price nodes.
	PatternRepository _patternRepository;

	// Clear helpers release only the objects that this controller owns.
	void clearMaterials(bool clearProblemVector);
	void clearProducts(bool clearProblemVector);
	void clearPatterns();

	// Solver helpers keep the raw Problem data synchronized with fresh CPLEX
	// models. Rebuilding is simpler and safer than trying to remove columns.
	void resetSolvers();
	void syncProblemToSolvers();
	void validateProblemReady();

	// Ryan-Foster decisions restrict the columns available at each node.
	bool patternContainsItem(Pattern* pattern, int itemIndex) const;
	bool isPatternCompatibleWithNode(const BPNode& node, Pattern* pattern) const;
	void applyRyanFosterConstraints(const BPNode& node, Subproblem& subproblem) const;
	string getItemDescription(int itemIndex) const;

	// Node column management and pricing helpers.
	double getPatternReducedCost(Pattern* pattern, const vector<double>& duals) const;
	void addActiveColumnToMaster(int repositoryIndex, MasterProblem& master,
		vector<int>& localToGlobalPatternIndices,
		unordered_set<int>& activePatternIndices) const;
	int addNegativePoolColumns(const BPNode& node, const vector<double>& duals,
		MasterProblem& master, vector<int>& localToGlobalPatternIndices,
		unordered_set<int>& activePatternIndices) const;
	void validateActivePatternIndices(const BPNode& node,
		const vector<int>& activePatternIndices) const;
	void requireExactPricingCertificate(const BPNode& node) const;

	// Branch-and-price search helpers.
	bool solveColumnGenerationAtNode(BPNode& node);
	bool evaluateBPNode(BPNode& node);
	bool isIntegerPatternSolution(const vector<double>& values) const;
	bool findRyanFosterPair(const BPNode& node, RyanFosterPair& pair) const;
	void createRyanFosterChildNodes(const BPNode& node, const RyanFosterPair& pair,
		BPNode& togetherNode, BPNode& separateNode);
	void reportRyanFosterBranch(const BPNode& node, const RyanFosterPair& pair,
		const BPNode& togetherNode, const BPNode& separateNode) const;
	void solveBranchAndPriceNode(const BPNode& node);
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
	void solveBP();	// Solve with Ryan-Foster branch-and-price.
	vector<Pattern* > findInitialPatterns(); // Generate one simple pattern per product.

};
