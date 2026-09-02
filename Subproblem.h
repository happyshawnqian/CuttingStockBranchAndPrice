#pragma once
#include "Problem.h"
#include "Utility.h"
#include <ilcplex\ilocplex.h>
#include <iostream>
using namespace std;

// Pricing subproblem for column generation and branch-and-price.
//
// Given dual values from the master exact-cover constraints, this model solves
// a binary knapsack problem and returns the minimum-reduced-cost item subset.
// The caller adds that subset only when its reduced cost is negative. Optional
// no-good constraints can exclude selected subsets from later pricing solves.
class Subproblem : public Problem
{
private:
	// CPLEX model for one knapsack pricing problem.
	IloEnv _env;
	IloModel _patGen;

	// Reduced cost objective:
	// material cost - sum_i dual_i * item_i_is_in_pattern.
	IloObjective _ReducedCost;

	// Binary decision variables a_i: whether unit-demand item i is in the roll.
	IloNumVarArray _Use;

	// Width capacity constraint for one raw roll.
	IloRange _Width;
	IloCplex _patSolver;

	// Validate two distinct in-range item indices or terminate.
	void validateItemPair(int firstItemIndex, int secondItemIndex) const;

public:
	// Create an empty CPLEX pricing model; initialize() adds variables and width.
	Subproblem();
	// Release the CPLEX environment and every Concert object created from it.
	~Subproblem();

	Subproblem(const Subproblem&) = delete;
	Subproblem& operator=(const Subproblem&) = delete;

	// Build binary item variables and the width constraint. The dual-dependent
	// objective is supplied separately by setObjective().
	void initialize();

	// Optional no-good constraints used to forbid an already generated pattern.
	// A forbidden pattern must differ in at least one item-selection bit.
	void addExcludedPattern(Pattern* pattern);
	// Add one no-good constraint for every supplied pattern.
	void addExcludedPatterns(const vector<Pattern* >& patterns);

	// Require both selected items to appear together or both to be absent. This
	// must mirror compatibility filtering for existing master columns.
	void addTogetherConstraint(int firstItemIndex, int secondItemIndex);
	// Forbid simultaneous selection of both items, mirroring the node's SEPARATE
	// compatibility rule for existing master columns.
	void addSeparateConstraint(int firstItemIndex, int secondItemIndex);

	// Replace the pricing objective with roll cost minus the supplied row duals.
	void setObjective(const vector<double>& duals);
	// Solve pricing and terminate if CPLEX cannot produce a solution.
	bool solve();
	// Solve pricing, optionally reporting failure as fatal; return false only when
	// failure reporting is disabled and no solution is available.
	bool solve(bool reportFailure);
	// Allocate a Pattern from the current binary pricing solution; the caller owns it.
	Pattern* getPattern();
	// Return the objective value of the current pricing solution.
	double getReducedCost();
	// Print the current reduced cost and selected item variables.
	void report();
};
