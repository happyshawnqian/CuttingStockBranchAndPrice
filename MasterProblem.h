#pragma once
#include <ilcplex\ilocplex.h>
#include "Pattern.h"
#include "PaperRoll.h"
#include "Problem.h"
#include "Utility.h"

// Restricted master problem for cutting stock.
//
// Rows require each distinct item to be covered exactly once, and each column
// represents one binary cutting pattern. During column generation the pattern
// variables are continuous and nonnegative. For a nonempty pattern, the
// exact-cover rows imply an upper bound of one. solveIP() converts the pattern
// variables to binary variables.
class MasterProblem : public Problem
{
private:
	// CPLEX environment and model objects. The environment must outlive every
	// Concert object created from it, so the class owns and ends it explicitly.
	IloEnv _env;
	IloModel _cutOpt;

	// Minimize real pattern cost plus any artificial-column penalty. When
	// artificial usage is zero and pattern costs are one, this equals the number
	// of raw rolls used.
	IloObjective _RollsUsed;

	// Set-partitioning rows. _Fill[i] requires item i to be covered exactly once.
	IloRangeArray _Fill;

	// Real pattern variables x_p. Branch-and-price normally instantiates only
	// node-active compatible columns; explicit bounds remain available to callers.
	IloNumVarArray _Cut;

	// Expensive fallback columns used in branch-and-price nodes. If any
	// artificial variable remains positive after column generation, the current
	// node is treated as infeasible for the real cutting-stock problem.
	IloNumVarArray _Artificial;
	IloCplex _cutSolver;

	// Prevent repeated IloConversion additions when solveIP() is called more
	// than once on the same master model.
	bool _integerConverted;

public:
	// Create an empty restricted master whose rows and columns are added later.
	MasterProblem();
	// Create an empty restricted master that references the supplied instance data.
	MasterProblem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products);
	// Release the CPLEX environment and every Concert object created from it.
	~MasterProblem();

	MasterProblem(const MasterProblem&) = delete;
	MasterProblem& operator=(const MasterProblem&) = delete;

	// Build exact-cover constraints. Columns are added later by addColumn().
	void initialize();

	// Add one expensive column per demand row so node LPs remain solvable while
	// pricing searches for real columns.
	void addArtificialColumns(double cost);

	// Add one nonnegative real pattern variable with no explicit upper bound.
	void addColumn(Pattern* pattern);
	// Add one real pattern variable with explicit bounds after validating that
	// the pattern is a non-duplicated binary subset of the product rows.
	void addColumn(Pattern* pattern, double lowerBound, double upperBound);
	// Add each supplied pattern with the default nonnegative variable bounds.
	void addColumns(const vector<Pattern* >& patterns);

	// Solve the current LP relaxation. Solver failure is fatal because controller
	// paths construct masters that are feasible by singleton or artificial columns.
	void solve();
	// Return exact-cover row duals in product-index order for pricing.
	vector<double> getDuals();
	// Return only real pattern values in local RMP column order.
	vector<double> getValues();
	// Return whether each real pattern variable is basic, preserving local _Cut
	// order. A missing basis status is treated as an internal solver error.
	vector<bool> getBasicColumnFlags() const;
	// Return the number of real pattern variables currently active in the RMP.
	int getRealColumnCount() const;
	// Remove one real pattern variable from the model and local _Cut array.
	// Artificial variables are stored separately and cannot be removed here.
	void removeColumn(int localColumnIndex);
	double getObjectiveValue();
	// Return total artificial-column usage; positive usage after exact pricing
	// proves that the node has no feasible solution using real patterns.
	double getArtificialUsage();
	// Print the LP objective, pattern values, reduced costs, bounds, and row duals.
	void report();
	// Convert all current real pattern variables to binary and solve the restricted
	// integer master. This does not generate additional columns.
	void solveIP();
	// Print the restricted integer-master objective and pattern values.
	void reportIP();

};
