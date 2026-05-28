#pragma once
#include <ilcplex\ilocplex.h>
#include "Pattern.h"
#include "PaperRoll.h"
#include "Problem.h"

class MasterProblem : public Problem	// master problem derived from problem
{
private:
	IloEnv _env;
	IloModel _cutOpt;
	IloObjective _RollsUsed;
	IloRangeArray _Fill;
	IloNumVarArray _Cut;
	IloNumVarArray _Artificial;
	IloCplex _cutSolver;
	bool _integerConverted;

public:
	MasterProblem();
	MasterProblem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products);
	~MasterProblem();

	MasterProblem(const MasterProblem&) = delete;
	MasterProblem& operator=(const MasterProblem&) = delete;

	void initialize();
	void addArtificialColumns(double cost);
	void addColumn(Pattern* pattern);	// add one column
	void addColumn(Pattern* pattern, double lowerBound, double upperBound);
	void addColumns(const vector<Pattern* >& patterns);	// add many columns
	bool solve();
	vector<double> getDuals();
	vector<double> getValues();
	double getObjectiveValue();
	double getArtificialUsage();
	void report();
	bool solveIP();
	void reportIP();

};
