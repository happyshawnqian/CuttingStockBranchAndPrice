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

	// Create an unevaluated root-level node with a new monotonic identifier.
	BPNode();

	// Create an unevaluated node at the specified tree depth with a new id.
	BPNode(int depth);

	// Add one canonical Ryan-Foster decision and invalidate any prior LP
	// evaluation. Repeating the same decision is harmless; conflicts are fatal.
	void addRyanFosterConstraint(int firstItemIndex, int secondItemIndex,
		RyanFosterBranchType branchType);

	// Replace the node's active global columns and invalidate its pricing
	// certificate because the stored LP values no longer describe the node.
	void setActivePatternIndices(const vector<int>& activePatternIndices);

	// Store an exact-priced RMP solution, its local-to-global column order,
	// pricing and cleanup statistics, then certify its objective as a lower bound.
	void setExactEvaluation(const vector<int>& activePatternIndices,
		const vector<double>& values, double objective, int sequence,
		int poolColumnCount, int exactColumnCount, int exactSolveCount,
		int columnCleanupCount, int deletedColumnCount,
		int peakActiveColumnCount);

	const vector<RyanFosterConstraint>& getRyanFosterConstraints() const;
	// Return global repository indices in the same local order as getValues().
	const vector<int>& getActivePatternIndices() const;
	// Return RMP values aligned with getActivePatternIndices().
	const vector<double>& getValues() const;
	int getId() const;
	// Return the node LP objective; it is a lower bound only after exact pricing.
	double getObjective() const;
	int getDepth() const;
	int getSequence() const;
	int getPoolColumnCount() const;
	int getExactColumnCount() const;
	int getExactSolveCount() const;
	int getColumnCleanupCount() const;
	int getDeletedColumnCount() const;
	int getPeakActiveColumnCount() const;
	// Report whether the stored LP objective was finalized by exact pricing.
	bool hasExactPricingCertificate() const;

	// Change the tree depth and invalidate any previously stored evaluation.
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
	int _columnCleanupCount;
	int _deletedColumnCount;
	int _peakActiveColumnCount;
	bool _hasExactPricingCertificate;
};

class BPNodeCompare
{
public:
	// Order exact-priced nodes for best-first search by LP objective, then depth
	// and insertion sequence. Passing an uncertified node is a fatal error.
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

	struct RmpColumnState
	{
		int localColumnIndex;
		int age;
		bool isBasic;
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
	int _rmpColumnLimitMultiplier;
	int _minRmpColumnAge;
	int _minRmpSolvesBetweenCleanups;

	// Global, append-only repository shared by all branch-and-price nodes.
	PatternRepository _patternRepository;

	// Delete controller-owned materials and optionally clear the Problem view.
	void clearMaterials(bool clearProblemVector);
	// Delete controller-owned products and optionally clear the Problem view.
	void clearProducts(bool clearProblemVector);
	// Delete all patterns owned by the global repository.
	void clearPatterns();

	// Replace both CPLEX wrappers with empty models, discarding accumulated state.
	void resetSolvers();
	// Copy the current Problem pointer vectors into both solver wrappers.
	void syncProblemToSolvers();
	// Validate the binary one-material cutting-stock assumptions or terminate.
	void validateProblemReady();

	// Return whether a binary pattern contains the requested unit-demand item.
	bool patternContainsItem(Pattern* pattern, int itemIndex) const;
	// Test an existing repository pattern against every branch decision at a node.
	bool isPatternCompatibleWithNode(const BPNode& node, Pattern* pattern) const;
	// Add the node's inherited Ryan-Foster decisions to an exact pricing model.
	void applyRyanFosterConstraints(const BPNode& node, Subproblem& subproblem) const;
	// Format an item index together with its source, copy, and width metadata.
	string getItemDescription(int itemIndex) const;

	// Compute a repository pattern's reduced cost from current master duals.
	double getPatternReducedCost(Pattern* pattern, const vector<double>& duals) const;
	// Add one global repository pattern to the local RMP with age zero and update
	// both mapping containers. Duplicate activation is an internal error.
	void addActiveColumnToMaster(int repositoryIndex, MasterProblem& master,
		vector<int>& localToGlobalPatternIndices,
		unordered_set<int>& activePatternIndices,
		vector<int>& columnAges) const;
	// Activate every inactive, node-compatible repository pattern with negative
	// reduced cost and return the number of local columns added.
	int addNegativePoolColumns(const BPNode& node, const vector<double>& duals,
		MasterProblem& master, vector<int>& localToGlobalPatternIndices,
		unordered_set<int>& activePatternIndices,
		vector<int>& columnAges) const;
	// Verify that the master, ordered mapping, membership set, and age vector
	// describe exactly the same active real columns.
	void validateLocalColumnState(const MasterProblem& master,
		const vector<int>& localToGlobalPatternIndices,
		const unordered_set<int>& activePatternIndices,
		const vector<int>& columnAges) const;
	// Reset basic-column ages and increment every valid nonbasic-column age.
	// Return the basis flags used by the current cleanup decision.
	vector<bool> updateColumnAges(const MasterProblem& master,
		vector<int>& columnAges) const;
	// Remove up to one row count of the oldest eligible nonbasic columns while
	// preserving all corresponding patterns in the global repository.
	int removeAgedColumnsFromMaster(MasterProblem& master,
		const vector<bool>& basicColumnFlags,
		vector<int>& localToGlobalPatternIndices,
		unordered_set<int>& activePatternIndices,
		vector<int>& columnAges) const;
	// Sort older RMP columns first, breaking equal ages by local index.
	static bool olderRmpColumnFirst(const RmpColumnState& left,
		const RmpColumnState& right);
	// Verify that active indices are unique, valid, and branch compatible.
	void validateActivePatternIndices(const BPNode& node,
		const vector<int>& activePatternIndices) const;
	// Terminate if a node objective has not been certified by exact pricing.
	void requireExactPricingCertificate(const BPNode& node) const;

	// Solve one node by repeated pool and exact pricing. Return false only when
	// exact pricing proves that artificial columns are still required.
	bool solveColumnGenerationAtNode(BPNode& node);
	// Exact-price, prune, or install an incumbent for one node. Return true only
	// when the resulting fractional node should remain in the open queue.
	bool evaluateBPNode(BPNode& node);
	// Test whether all local master values are integral within RC_EPS.
	bool isIntegerPatternSolution(const vector<double>& values) const;
	// Select the fractional Ryan-Foster pair whose together value is closest to
	// 0.5. Return false when no eligible pair exists.
	bool findRyanFosterPair(const BPNode& node, RyanFosterPair& pair) const;
	// Build complementary children and inherit only parent columns compatible
	// with each child's complete Ryan-Foster path.
	void createRyanFosterChildNodes(const BPNode& node, const RyanFosterPair& pair,
		BPNode& togetherNode, BPNode& separateNode);
	// Print one Ryan-Foster decision and the ids assigned to both children.
	void reportRyanFosterBranch(const BPNode& node, const RyanFosterPair& pair,
		const BPNode& togetherNode, const BPNode& separateNode) const;
	// Run best-first branching from an initial node using only exact-certified
	// objectives as global lower-bound candidates.
	void solveBranchAndPriceNode(const BPNode& node);
	bool hasBranchAndPriceIncumbent() const;
	// Test whether ceil(global LP bound) reaches the current integer incumbent.
	bool isIntegerBoundClosed() const;
	// Print the current LP lower bound, integer lower bound, and incumbent.
	void reportBranchAndPriceBounds() const;
	// Print the incumbent solution, global bounds, repository size, and stop reason.
	void reportBranchAndPriceSolution();

public:
	// Create and own an empty Problem together with fresh master/pricing models.
	Controller();
	// Borrow an external Problem while owning only the solver wrappers.
	Controller(Problem* problem);
	// Release the repository and solvers plus any Problem and input data it owns.
	~Controller();

	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

	// Replace the current instance with a borrowed Problem and rebuild all solver
	// state; previously generated patterns and owned input data are discarded.
	void setProblem(Problem* problem);
	Problem* getProblem() { return _problem; }
	// Configure the active real-column limit as a multiple of master row count.
	void setRmpColumnLimitMultiplier(int multiplier);
	// Configure the minimum nonbasic age required for column deletion.
	void setMinRmpColumnAge(int minAge);
	// Configure the number of successful RMP solves required between cleanups.
	void setMinRmpSolvesBetweenCleanups(int solveCount);

	// Load and own raw-roll records from dataFile + "materials.json", where
	// dataFile is an optional directory prefix.
	void loadMaterials(string dataFile);
	// Load products from dataFile + "products.json", where dataFile is an optional
	// directory prefix, and expand demand into distinct controller-owned items.
	void loadProducts(string dataFile);

	// Solve only the LP relaxation by standalone column generation.
	void solveCG();
	// Solve an integer restricted master over columns generated by solveCG().
	void solveIP();
	// Solve the binary cutting-stock model with Ryan-Foster branch-and-price.
	void solveBP();
	// Allocate one singleton pattern per unit-demand product; the caller takes
	// ownership of the returned pointers.
	vector<Pattern* > findInitialPatterns();

};
