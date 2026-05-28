#include "Subproblem.h"
#include <cstdlib>

Subproblem::Subproblem() : Problem()
{
	_patGen = IloModel(_env, "patGenerator");
	_ReducedCost = IloAdd(_patGen, IloMinimize(_env));
	_Use = IloNumVarArray(_env);
	_patSolver = IloCplex(_patGen);
}

Subproblem::~Subproblem()
{
	_env.end();
}

void Subproblem::initialize()
{
	if (_materials.empty() || _products.empty())
	{
		cout << "Error, subproblem requires materials and products" << endl;
		exit(1);
	}
	int rollWidth = _materials[0]->getWidth();

	// create variables
	for (int i = 0; i < static_cast<int>(_products.size()); i++)
	{
		PaperRoll* product = _products[i];
		string varName = "a_" + to_string(product->getId());
		_Use.add(IloNumVar(_env, 0, IloInfinity, ILOINT, varName.c_str()));
	}

	// create constraint
	IloExpr exp(_env);
	for (int i = 0; i < static_cast<int>(_products.size()); i++)
	{
		PaperRoll* product = _products[i];
		exp += product->getWidth() * _Use[i];
	}
	string conName = "conWidth";
	_Width = IloRange(_env, -IloInfinity, exp, rollWidth, conName.c_str());
	exp.end();
	_patGen.add(_Width);
}

void Subproblem::setObjective(const vector<double>& duals)
{
	int nWdth = static_cast<int>(_products.size());
	if (static_cast<int>(duals.size()) != nWdth)
	{
		cout << "error, duals and products size not match" << endl;
		exit(1);
	}
	IloExpr objExp(_env);
	objExp += _materials[0]->getCost();
	for (int i = 0; i < nWdth; i++)
	{
		objExp -= duals[i] * _Use[i];
	}
	_ReducedCost.setExpr(objExp);
	objExp.end();
}

void Subproblem::solve()
{
	//_patSolver.exportModel("subproblem.lp");
	if (!_patSolver.solve())
	{
		cout << "Error, subproblem could not be solved" << endl;
		exit(1);
	}
}

Pattern* Subproblem::getPattern()
{
	int nWdth = static_cast<int>(_products.size());
	IloNumArray newPatt(_env, nWdth);
	_patSolver.getValues(newPatt, _Use);

	vector<pair<int, int>> content;
	for (int i = 0; i < nWdth; i++)
	{
		if (newPatt[i] > Utility::RC_EPS)
		{
			int value = static_cast<int>(newPatt[i] + 0.5);
			if (value > 0)
			{
				content.push_back(make_pair(i, value));
			}
		}
	}
	newPatt.end();

	Pattern* newPattern = new Pattern();
	newPattern->setContent(content);
	newPattern->setCost(_materials[0]->getCost());

	return newPattern;
}

double Subproblem::getReducedCost()
{
	return _patSolver.getValue(_ReducedCost);
}

void Subproblem::report()
{
	cout << endl;
	cout << "Reduced cost is " << _patSolver.getValue(_ReducedCost) << endl;
	cout << endl;

	if (_patSolver.getValue(_ReducedCost) <= -Utility::RC_EPS) {
		for (IloInt i = 0; i < _Use.getSize(); i++)  {
			cout << "  Use" << i << " = " << _patSolver.getValue(_Use[i]) << endl;
		}
		cout << endl;
	}
}
