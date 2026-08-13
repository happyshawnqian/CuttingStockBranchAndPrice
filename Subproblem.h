#pragma once
#include "Problem.h"
#include "Utility.h"
#include <ilcplex\ilocplex.h>
#include <iostream>
using namespace std;

// Pricing subproblem for column generation and branch-and-price.
//
// Given dual values from the master exact-cover constraints, this model solves
// a binary knapsack problem and returns an item subset with negative reduced
// cost. No-good constraints prevent regeneration of known subsets.
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

public:
	Subproblem();
	~Subproblem();

	Subproblem(const Subproblem&) = delete;
	Subproblem& operator=(const Subproblem&) = delete;

	void initialize();	// create variables and constraint, note that objective is not set

	// Optional no-good constraints used to forbid an already generated pattern.
	// A forbidden pattern must differ in at least one item-selection bit.
	void addExcludedPattern(Pattern* pattern);
	void addExcludedPatterns(const vector<Pattern* >& patterns);

	void setObjective(const vector<double>& duals);	// get duals from master problem, and set objective
	bool solve();
	bool solve(bool reportFailure);
	Pattern* getPattern();	// get the newly generated pattern
	double getReducedCost();
	void report();
};
