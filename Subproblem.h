#pragma once
#include "Problem.h"
#include "Utility.h"
#include <ilcplex\ilocplex.h>
#include <iostream>
using namespace std;

// Pricing subproblem for the column-generation-only path.
//
// Given dual values from the master demand constraints, this model solves a
// bounded knapsack problem and returns a cutting pattern with negative reduced
// cost. The branch-and-price path currently uses deterministic enumeration in
// Controller, but this class is still used by solveCG().
class Subproblem : public Problem
{
private:
	// CPLEX model for one knapsack pricing problem.
	IloEnv _env;
	IloModel _patGen;

	// Reduced cost objective:
	// material cost - sum_i dual_i * number_of_product_i_in_pattern.
	IloObjective _ReducedCost;

	// Integer decision variables a_i: how many pieces of product i are cut from
	// one raw roll.
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
	// A forbidden pattern must differ in at least one product count.
	void addExcludedPattern(Pattern* pattern);
	void addExcludedPatterns(const vector<Pattern* >& patterns);

	void setObjective(const vector<double>& duals);	// get duals from master problem, and set objective
	bool solve();
	bool solve(bool reportFailure);
	Pattern* getPattern();	// get the newly generated pattern
	double getReducedCost();
	void report();
};
