#include "MasterProblem.h"
#include <cstdlib>

MasterProblem::MasterProblem() : Problem()
{
	// Build an empty restricted master. Demand rows and pattern columns are
	// added later so the same class can be used by different algorithms.
	_cutOpt = IloModel(_env, "cutStock");
	_RollsUsed = IloAdd(_cutOpt, IloMinimize(_env));
	_Fill = IloRangeArray(_env);
	_Cut = IloNumVarArray(_env);
	_Artificial = IloNumVarArray(_env);
	_cutSolver = IloCplex(_cutOpt);
	_integerConverted = false;
}

// Build the same empty RMP while storing non-owning views of the instance data.
MasterProblem::MasterProblem(const vector<PaperRoll* >& materials, const vector<PaperRoll* >& products) : Problem(materials, products)
{
	_cutOpt = IloModel(_env, "cutStock");
	_RollsUsed = IloAdd(_cutOpt, IloMinimize(_env));
	_Fill = IloRangeArray(_env);
	_Cut = IloNumVarArray(_env);
	_Artificial = IloNumVarArray(_env);
	_cutSolver = IloCplex(_cutOpt);
	_integerConverted = false;
}

// Ending the environment releases all model, variable, constraint, and solver handles.
MasterProblem::~MasterProblem()
{
	_env.end();
}

// Create one equality row per expanded unit-demand product; columns are added later.
void MasterProblem::initialize()
{
	int nWdth = static_cast<int>(_products.size());	// number of unit-demand items
	if (_materials.empty() || _products.empty())
	{
		cout << "Error, master problem requires materials and products" << endl;
		exit(1);
	}
	
	// Set-partitioning constraints:
	//   sum_p a_ip * x_p = 1
	// where a_ip is binary and indicates whether pattern p contains item i.
	for (int i = 0; i < nWdth; i++)
	{
		PaperRoll* product = _products[i];
		if (product == nullptr || product->getNumber() != 1)
		{
			cout << "Error, master problem requires unit-demand products" << endl;
			exit(1);
		}
		string conName = "conProduct_" + to_string(product->getId());
		_Fill.add(IloRange(_env, 1, 1, conName.c_str()));
	}

	_cutOpt.add(_Fill);	// add constraints to model
}

void MasterProblem::addArtificialColumns(double cost)
{
	// Artificial columns are identity columns with a large objective cost.
	// They keep the LP feasible while branch-and-price is still generating
	// real columns. Positive artificial usage after pricing means the node is
	// infeasible in the real model.
	for (int i = 0; i < _Fill.getSize(); i++)
	{
		IloNumColumn col(_env);
		col += _RollsUsed(cost);
		col += _Fill[i](1);

		string varName = "artificial_" + to_string(i);
		_Artificial.add(IloNumVar(col, 0, 1, ILOFLOAT, varName.c_str()));
		col.end();
	}
}

// Delegate to the bounded overload using the standard nonnegative continuous domain.
void MasterProblem::addColumn(Pattern* pattern)
{
	addColumn(pattern, 0, IloInfinity);
}

void MasterProblem::addColumn(Pattern* pattern, double lowerBound, double upperBound)
{
	// Real pattern variables are nonnegative and normally have no explicit upper
	// bound. Exact-cover rows implicitly keep every nonempty binary pattern at
	// or below one. Explicit bounds remain available for specialized callers.
	if (lowerBound < 0 || upperBound < lowerBound)
	{
		cout << "Error, invalid bounds for a master pattern variable" << endl;
		exit(1);
	}

	IloNumColumn col(_env);
	col += _RollsUsed(pattern->getCost());
	vector<bool> included(_Fill.getSize(), false);
	for (auto content : pattern->getContent())
	{
		if (content.first < 0 || content.first >= _Fill.getSize()
			|| content.second != 1 || included[content.first])
		{
			cout << "Error, a master pattern must be a binary subset of the products" << endl;
			col.end();
			exit(1);
		}
		included[content.first] = true;
		col += _Fill[content.first](1);
	}

	string varName = "x_" + to_string(pattern->getId());
	_Cut.add(IloNumVar(col, lowerBound, upperBound, ILOFLOAT, varName.c_str()));
	col.end();
}

// Preserve caller order while applying the standard bounds to each pattern.
void MasterProblem::addColumns(const vector<Pattern* >& patterns)
{
	for (auto pattern : patterns)
	{
		addColumn(pattern);
	}
}

// Suppress normal CPLEX output and fail fast because all controller-created RMPs
// have a feasible basis from singleton or artificial columns.
void MasterProblem::solve()
{
	//_cutSolver.exportModel("masterProblem.lp");
	_cutSolver.setOut(_env.getNullStream());
	if (!_cutSolver.solve())
	{
		cout << "Error, master problem could not be solved" << endl;
		exit(1);
	}

#ifdef DEBUG
	report();
#endif

}

vector<double> MasterProblem::getDuals()
{
	// Dual prices of exact-cover constraints are passed to the pricing problem.
	// A new pattern is profitable when cost - dual contribution is negative.
	vector<double> duals;
	for (int i = 0; i < _Fill.getSize(); i++)
	{
		duals.push_back(_cutSolver.getDual(_Fill[i]));
	}
	return duals;
}

vector<double> MasterProblem::getValues()
{
	// Return only the real pattern variable values. Artificial variables are
	// excluded because they do not correspond to cutting patterns.
	vector<double> values;
	for (int j = 0; j < _Cut.getSize(); j++)
	{
		values.push_back(_cutSolver.getValue(_Cut[j]));
	}
	return values;
}

vector<bool> MasterProblem::getBasicColumnFlags() const
{
	// Basis flags use the same local order as values and the controller's
	// local-to-global mapping. Only a valid post-solve basis may drive aging.
	vector<bool> basicColumnFlags;
	for (int j = 0; j < _Cut.getSize(); j++)
	{
		IloCplex::BasisStatus status = _cutSolver.getBasisStatus(_Cut[j]);
		if (status == IloCplex::NotABasicStatus)
		{
			cout << "Error, master pattern variable has no valid basis status" << endl;
			exit(1);
		}
		basicColumnFlags.push_back(status == IloCplex::Basic);
	}
	return basicColumnFlags;
}

int MasterProblem::getRealColumnCount() const
{
	return static_cast<int>(_Cut.getSize());
}

void MasterProblem::removeColumn(int localColumnIndex)
{
	// Column deletion is supported only while the restricted master is an LP.
	// Removing the model extractable leaves artificial variables untouched.
	if (_integerConverted)
	{
		cout << "Error, cannot remove a column after integer conversion" << endl;
		exit(1);
	}
	if (localColumnIndex < 0 || localColumnIndex >= _Cut.getSize())
	{
		cout << "Error, invalid local master column index "
			<< localColumnIndex << endl;
		exit(1);
	}

	IloNumVar removedColumn = _Cut[localColumnIndex];
	_cutOpt.remove(removedColumn);
	_Cut.remove(localColumnIndex);
	removedColumn.end();
}

double MasterProblem::getObjectiveValue()
{
	return _cutSolver.getObjValue();
}

double MasterProblem::getArtificialUsage()
{
	// Summing identity-column values measures total uncovered demand in the real RMP.
	double usage = 0;
	for (int j = 0; j < _Artificial.getSize(); j++)
	{
		usage += _cutSolver.getValue(_Artificial[j]);
	}
	return usage;
}

void MasterProblem::report()
{
	// Keep local column diagnostics together with the exact-cover duals used by pricing.
	cout << "Master Problem report after solved" << endl;
	cout << "Using " << _cutSolver.getObjValue()
		<< " rolls" << endl;
	cout << endl;

	cout << "Pattern variables:" << endl;

	for (int j = 0; j < _Cut.getSize(); j++)
	{
		double value = _cutSolver.getValue(_Cut[j]);
		double lowerBound = _Cut[j].getLB();
		double upperBound = _Cut[j].getUB();
		double reducedCost = _cutSolver.getReducedCost(_Cut[j]);

		cout << "  "
			<< _Cut[j].getName()
			<< " = " << value
			<< ", reduced cost = " << reducedCost
			<< ", bounds = ["
			<< lowerBound << ", ";

		if (upperBound >= IloInfinity / 2)
		{
			cout << "+infinity";
		}
		else
		{
			cout << upperBound;
		}

		cout << "]" << endl;
	}

	cout << endl;

	cout << "Exact-cover constraint duals:" << endl;

	for (int i = 0; i < _Fill.getSize(); i++)
	{
		cout << "  Fill" << i
			<< " dual = "
			<< _cutSolver.getDual(_Fill[i])
			<< endl;
	}

	cout << endl;
}

void MasterProblem::solveIP()
{
	// Convert the current restricted master columns to integer variables. This
	// is not full branch-and-price; it only solves over columns already present.
	if (!_integerConverted)
	{
		_cutOpt.add(IloConversion(_env, _Cut, ILOBOOL));
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
	// The integer report uses local RMP column positions because it is diagnostic only.
	cout << endl;
	cout << "Best integer solution uses "
		<< _cutSolver.getObjValue() << " rolls" << endl;
	cout << endl;
	for (int j = 0; j < _Cut.getSize(); j++) {
		cout << "  Cut" << j << " = " << _cutSolver.getValue(_Cut[j]) << endl;
	}
}
