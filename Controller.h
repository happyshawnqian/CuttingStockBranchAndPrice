#pragma once
#include "problem.h"
#include "json\json.h"
#include <fstream>
#include "MasterProblem.h"
#include "Subproblem.h"
using namespace std;

class Controller
{
private:
	Problem* _problem;
	MasterProblem* _masterProblem;
	Subproblem* _subproblem;
	bool _ownsProblem;
	bool _ownsMaterials;
	bool _ownsProducts;

	vector<Pattern* > _patterns;

	void clearMaterials(bool clearProblemVector);
	void clearProducts(bool clearProblemVector);
	void clearPatterns();
	void resetSolvers();
	void syncProblemToSolvers();
	void validateProblemReady();

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

	void solveCG();	// solve by column generation
	void solveIP();	// solve as an IP by generated columns
	vector<Pattern* > findInitialPatterns(); // generate initial patterns

};
