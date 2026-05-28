#include "MasterProblem.h"
#include <cstdlib>

MasterProblem::MasterProblem() : Problem()
{
	_cutOpt = IloModel(_env, "cutStock");
	_RollsUsed = IloAdd(_cutOpt, IloMinimize(_env));
	_Fill = IloRangeArray(_env);
	_Cut = IloNumVarArray(_env);
	_cutSolver = IloCplex(_cutOpt);
	_integerConverted = false;
}

MasterProblem::MasterProblem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products) : Problem(materials, products)
{
	_cutOpt = IloModel(_env, "cutStock");
	_RollsUsed = IloAdd(_cutOpt, IloMinimize(_env));
	_Fill = IloRangeArray(_env);
	_Cut = IloNumVarArray(_env);
	_cutSolver = IloCplex(_cutOpt);
	_integerConverted = false;
}

MasterProblem::~MasterProblem()
{
	_env.end();
}

void MasterProblem::initialize()
{
	int nWdth = static_cast<int>(_products.size());	// number of products
	if (_materials.empty() || _products.empty())
	{
		cout << "Error, master problem requires materials and products" << endl;
		exit(1);
	}
	
	// create constraints
	for (int i = 0; i < nWdth; i++)
	{
		PaperRoll* product = _products[i];
		string conName = "conProduct_" + to_string(product->getId());
		_Fill.add(IloRange(_env, product->getNumber(), IloInfinity, conName.c_str()));
	}

	_cutOpt.add(_Fill);	// add constraints to model
}

void MasterProblem::addColumn(Pattern* pattern)
{
	// prepare column expression
	IloNumColumn col(_env);
	col += _RollsUsed(pattern->getCost());
	for (auto content : pattern->getContent())
	{
		col += _Fill[content.first](content.second);
	}

	string varName = "x_" + to_string(pattern->getId());
	_Cut.add(IloNumVar(col, 0, IloInfinity, ILOFLOAT, varName.c_str()));
	col.end();
}

void MasterProblem::addColumns(const vector<Pattern* >& patterns)
{
	for (auto pattern : patterns)
	{
		addColumn(pattern);
	}
}

void MasterProblem::solve()
{
	//_cutSolver.exportModel("masterProblem.lp");
	if (!_cutSolver.solve())
	{
		cout << "Error, master problem could not be solved" << endl;
		exit(1);
	}
}

vector<double> MasterProblem::getDuals()
{
	vector<double> duals;
	for (int i = 0; i < _Fill.getSize(); i++)
	{
		duals.push_back(_cutSolver.getDual(_Fill[i]));
	}
	return duals;
}

void MasterProblem::report()
{
	cout << endl;
	cout << "Using " << _cutSolver.getObjValue() << " rolls" << endl;
	cout << endl;

	for (int j = 0; j < _Cut.getSize(); j++)
	{
		cout << "  Cut" << j << " = " << _cutSolver.getValue(_Cut[j]) << endl;
	}
	cout << endl;
	for (int i = 0; i < _Fill.getSize(); i++) {
		cout << "  Fill" << i << " = " << _cutSolver.getDual(_Fill[i]) << endl;
	}
	cout << endl;
}

void MasterProblem::solveIP()
{
	if (!_integerConverted)
	{
		_cutOpt.add(IloConversion(_env, _Cut, ILOINT));
		_integerConverted = true;
	}
	if (!_cutSolver.solve())
	{
		cout << "Error, integer master problem could not be solved" << endl;
		exit(1);
	}
}

void MasterProblem::reportIP()
{
	cout << endl;
	cout << "Best integer solution uses "
		<< _cutSolver.getObjValue() << " rolls" << endl;
	cout << endl;
	for (int j = 0; j < _Cut.getSize(); j++) {
		cout << "  Cut" << j << " = " << _cutSolver.getValue(_Cut[j]) << endl;
	}
}
