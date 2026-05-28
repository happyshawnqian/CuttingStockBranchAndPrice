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

void Subproblem::addExcludedPattern(Pattern* pattern)
{
	if (pattern == nullptr || _materials.empty() || _products.empty()) return;

	int nWdth = static_cast<int>(_products.size());
	int rollWidth = _materials[0]->getWidth();
	vector<int> target(nWdth, 0);
	for (auto content : pattern->getContent())
	{
		if (content.first >= 0 && content.first < nWdth)
		{
			target[content.first] += content.second;
		}
	}

	IloExpr differs(_env);
	for (int i = 0; i < nWdth; i++)
	{
		int maxUse = rollWidth / _products[i]->getWidth();
		string varName = "diff_" + to_string(pattern->getId()) + "_" + to_string(i);
		IloBoolVar diff(_env, varName.c_str());
		differs += diff;
		_patGen.add(_Use[i] - target[i] <= maxUse * diff);
		_patGen.add(target[i] - _Use[i] <= maxUse * diff);
	}
	_patGen.add(differs >= 1);
	differs.end();
}

void Subproblem::addExcludedPatterns(const vector<Pattern* >& patterns)
{
	for (auto pattern : patterns)
	{
		addExcludedPattern(pattern);
	}
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

bool Subproblem::solve()
{
	return solve(true);
}

bool Subproblem::solve(bool reportFailure)
{
	//_patSolver.exportModel("subproblem.lp");
	if (!_patSolver.solve())
	{
		if (reportFailure)
		{
			cout << "Error, subproblem could not be solved" << endl;
		}
		return false;
	}
	return true;
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
