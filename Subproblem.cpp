#include "Subproblem.h"
#include <cstdlib>

Subproblem::Subproblem() : Problem()
{
	// The pricing model is initialized without variables/constraints until
	// materials and products are available.
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
	// Create one binary variable per unit-demand item. A feasible solution is a
	// subset of distinct items that fits within the raw roll width.
	if (_materials.empty() || _products.empty())
	{
		cout << "Error, subproblem requires materials and products" << endl;
		exit(1);
	}
	int rollWidth = _materials[0]->getWidth();

	// a_i = 1 exactly when item i belongs to this candidate pattern.
	for (int i = 0; i < static_cast<int>(_products.size()); i++)
	{
		PaperRoll* product = _products[i];
		if (product == nullptr || product->getNumber() != 1)
		{
			cout << "Error, subproblem requires unit-demand products" << endl;
			exit(1);
		}
		string varName = "a_" + to_string(product->getId());
		_Use.add(IloNumVar(_env, 0, 1, ILOBOOL, varName.c_str()));
	}

	// Width capacity: sum_i width_i * a_i <= raw_roll_width.
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

void Subproblem::validateItemPair(int firstItemIndex, int secondItemIndex) const
{
	int itemCount = static_cast<int>(_products.size());
	if (firstItemIndex < 0 || firstItemIndex >= itemCount
		|| secondItemIndex < 0 || secondItemIndex >= itemCount
		|| firstItemIndex == secondItemIndex)
	{
		cout << "Error, invalid item pair for a Ryan-Foster constraint" << endl;
		exit(1);
	}
}

void Subproblem::addTogetherConstraint(int firstItemIndex, int secondItemIndex)
{
	validateItemPair(firstItemIndex, secondItemIndex);

	// Together branch: a feasible pattern contains both items or neither item.
	// With binary selection variables, a_i - a_j = 0 enforces this equivalence.
	IloExpr relation(_env);
	relation += _Use[firstItemIndex] - _Use[secondItemIndex];
	string constraintName = "rfTogether_" + to_string(firstItemIndex)
		+ "_" + to_string(secondItemIndex);
	_patGen.add(IloRange(_env, 0, relation, 0, constraintName.c_str()));
	relation.end();
}

void Subproblem::addSeparateConstraint(int firstItemIndex, int secondItemIndex)
{
	validateItemPair(firstItemIndex, secondItemIndex);

	// Separate branch: no feasible pattern may contain both items.
	// The inequality a_i + a_j <= 1 removes their simultaneous selection.
	IloExpr relation(_env);
	relation += _Use[firstItemIndex] + _Use[secondItemIndex];
	string constraintName = "rfSeparate_" + to_string(firstItemIndex)
		+ "_" + to_string(secondItemIndex);
	_patGen.add(IloRange(_env, -IloInfinity, relation, 1, constraintName.c_str()));
	relation.end();
}

void Subproblem::addExcludedPattern(Pattern* pattern)
{
	// Exclude exactly one binary item subset with a Hamming-distance constraint:
	//   sum(i in S)(1 - a_i) + sum(i not in S)(a_i) >= 1.
	if (pattern == nullptr || _materials.empty() || _products.empty()) return;

	int nWdth = static_cast<int>(_products.size());
	vector<int> target(nWdth, 0);
	for (auto content : pattern->getContent())
	{
		if (content.first >= 0 && content.first < nWdth)
		{
			target[content.first] = 1;
		}
	}

	IloExpr differs(_env);
	for (int i = 0; i < nWdth; i++)
	{
		if (target[i] == 1)
		{
			differs += 1 - _Use[i];
		}
		else
		{
			differs += _Use[i];
		}
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
	// Reduced cost for a generated column:
	//   roll_cost - sum_i dual_i * a_i.
	// A negative value means adding the pattern can improve the master LP.
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
	//cout << "============ Subproblem to solve ============" << endl;
	_patSolver.setOut(_env.getNullStream());
	// No-good constraints may be added between pricing solves, invalidating MIP
	// starts retained by CPLEX. Clear retained starts before each re-optimization
	// to avoid retrying stale solutions and emitting repeated warnings.
	int numMIPStarts = _patSolver.getNMIPStarts();
	if (numMIPStarts > 0)
	{
		_patSolver.deleteMIPStarts(0, numMIPStarts);
	}
	if (!_patSolver.solve())
	{
		if (reportFailure)
		{
			cout << "Error, subproblem could not be solved" << endl;
			exit(1);
		}
		return false;
	}
	return true;
}

Pattern* Subproblem::getPattern()
{
	// Convert the binary pricing solution back into the Pattern abstraction.
	int nWdth = static_cast<int>(_products.size());
	IloNumArray newPatt(_env, nWdth);
	_patSolver.getValues(newPatt, _Use);

	vector<pair<int, int>> content;
	for (int i = 0; i < nWdth; i++)
	{
		if (newPatt[i] > Utility::RC_EPS)
		{
			content.push_back(make_pair(i, 1));
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
	// This reports the pricing objective, i.e. reduced cost, not the master
	// problem objective. Negative values indicate improving columns.
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
