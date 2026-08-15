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
// variables are continuous and nonnegative. Their upper bound of one is implied
// by the exact-cover rows. solveIP() converts them to binary variables.
class MasterProblem : public Problem
{
private:
	// CPLEX environment and model objects. The environment must outlive every
	// Concert object created from it, so the class owns and ends it explicitly.
	IloEnv _env;
	IloModel _cutOpt;

	// Objective: minimize the total cost / number of raw rolls used.
	IloObjective _RollsUsed;

	// Set-partitioning rows. _Fill[i] requires item i to be covered exactly once.
	IloRangeArray _Fill;

	// Real pattern variables x_p. Compatible columns are nonnegative without an
	// explicit upper bound; branch-incompatible columns can be fixed at zero.
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
	MasterProblem();
	MasterProblem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products);
	~MasterProblem();

	MasterProblem(const MasterProblem&) = delete;
	MasterProblem& operator=(const MasterProblem&) = delete;

	// Build exact-cover constraints. Columns are added later by addColumn().
	void initialize();

	// Add one expensive column per demand row so node LPs remain solvable while
	// pricing searches for real columns.
	void addArtificialColumns(double cost);

	void addColumn(Pattern* pattern);	// add one column
	void addColumn(Pattern* pattern, double lowerBound, double upperBound);
	void addColumns(const vector<Pattern* >& patterns);	// add many columns

	// Solve the current LP relaxation and expose primal/dual information used
	// by pricing and branching.
	bool solve();
	vector<double> getDuals();
	vector<double> getValues();
	double getObjectiveValue();
	double getArtificialUsage();
	void report();
	bool solveIP();
	void reportIP();

};
