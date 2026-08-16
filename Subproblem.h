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

	void validateItemPair(int firstItemIndex, int secondItemIndex) const;

public:
	Subproblem();
	~Subproblem();

	Subproblem(const Subproblem&) = delete;
	Subproblem& operator=(const Subproblem&) = delete;

	// Build binary item variables and the width constraint. The dual-dependent
	// objective is supplied separately by setObjective().
	void initialize();

	// Optional no-good constraints used to forbid an already generated pattern.
	// A forbidden pattern must differ in at least one item-selection bit.
	void addExcludedPattern(Pattern* pattern);
	void addExcludedPatterns(const vector<Pattern* >& patterns);

	// Pricing-side Ryan-Foster restrictions. These constraints must mirror the
	// compatibility rules used for existing columns in the node master.
	void addTogetherConstraint(int firstItemIndex, int secondItemIndex);
	void addSeparateConstraint(int firstItemIndex, int secondItemIndex);

	void setObjective(const vector<double>& duals); // Set reduced cost from master duals.
	bool solve();
	bool solve(bool reportFailure);
	Pattern* getPattern(); // Convert the current pricing solution to a candidate pattern.
	double getReducedCost();
	void report();
};
