#pragma once
#include "Problem.h"
#include "Utility.h"
#include <ilcplex\ilocplex.h>
#include <iostream>
using namespace std;

class Subproblem : public Problem
{
private:
	IloEnv _env;
	IloModel _patGen;
	IloObjective _ReducedCost;
	IloNumVarArray _Use;
	IloRange _Width;
	IloCplex _patSolver;

public:
	Subproblem();
	~Subproblem();

	Subproblem(const Subproblem&) = delete;
	Subproblem& operator=(const Subproblem&) = delete;

	void initialize();	// create variables and constraint, note that objective is not set
	void setObjective(const vector<double>& duals);	// get duals from master problem, and set objective
	void solve();
	Pattern* getPattern();	// get the newly generated pattern
	double getReducedCost();
	void report();
};
