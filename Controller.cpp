#include "Controller.h"
#include <cstdlib>

Controller::Controller()
{
	_problem = new Problem();
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = true;
	_ownsMaterials = false;
	_ownsProducts = false;
}

Controller::Controller(Problem* problem)
{
	_problem = problem;
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = false;
	_ownsMaterials = false;
	_ownsProducts = false;

	syncProblemToSolvers();
}

Controller::~Controller()
{
	clearPatterns();
	if (_ownsProducts) clearProducts(true);
	if (_ownsMaterials) clearMaterials(true);

	if (_ownsProblem)
	{
		delete _problem;
	}
	delete _masterProblem;
	delete _subproblem;
}

void Controller::setProblem(Problem* problem)
{
	clearPatterns();
	if (_ownsProducts) clearProducts(true);
	if (_ownsMaterials) clearMaterials(true);
	if (_ownsProblem)
	{
		delete _problem;
	}
	_problem = problem;
	_ownsProblem = false;
	_ownsMaterials = false;
	_ownsProducts = false;

	resetSolvers();
	syncProblemToSolvers();
}

void Controller::syncProblemToSolvers()
{
	if (_problem != nullptr)
	{
		_masterProblem->setMaterials(_problem->getMaterials());
		_masterProblem->setProducts(_problem->getProducts());
		_subproblem->setMaterials(_problem->getMaterials());
		_subproblem->setProducts(_problem->getProducts());
	}
}

void Controller::resetSolvers()
{
	delete _masterProblem;
	delete _subproblem;

	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
}

void Controller::clearMaterials(bool clearProblemVector)
{
	if (_problem != nullptr)
	{
		vector<PaperRoll* > materials = _problem->getMaterials();
		if (_ownsMaterials)
		{
			for (auto material : materials)
			{
				delete material;
			}
		}
		if (clearProblemVector)
		{
			_problem->setMaterials(vector<PaperRoll* >());
		}
	}

	_masterProblem->setMaterials(vector<PaperRoll* >());
	_subproblem->setMaterials(vector<PaperRoll* >());
	_ownsMaterials = false;
}

void Controller::clearProducts(bool clearProblemVector)
{
	if (_problem != nullptr)
	{
		vector<PaperRoll* > products = _problem->getProducts();
		if (_ownsProducts)
		{
			for (auto product : products)
			{
				delete product;
			}
		}
		if (clearProblemVector)
		{
			_problem->setProducts(vector<PaperRoll* >());
		}
	}

	_masterProblem->setProducts(vector<PaperRoll* >());
	_subproblem->setProducts(vector<PaperRoll* >());
	_ownsProducts = false;
}

void Controller::clearPatterns()
{
	for (auto pattern : _patterns)
	{
		delete pattern;
	}
	_patterns.clear();
}

void Controller::validateProblemReady()
{
	if (_problem == nullptr)
	{
		cout << "Error, problem is not set" << endl;
		exit(1);
	}

	vector<PaperRoll* > materials = _problem->getMaterials();
	vector<PaperRoll* > products = _problem->getProducts();
	if (materials.empty())
	{
		cout << "Error, no material rolls were loaded" << endl;
		exit(1);
	}
	if (products.empty())
	{
		cout << "Error, no product rolls were loaded" << endl;
		exit(1);
	}
	if (materials[0] == nullptr || materials[0]->getWidth() <= 0)
	{
		cout << "Error, material width must be positive" << endl;
		exit(1);
	}

	int materialWidth = materials[0]->getWidth();
	for (auto product : products)
	{
		if (product == nullptr || product->getWidth() <= 0)
		{
			cout << "Error, product width must be positive" << endl;
			exit(1);
		}
		if (product->getNumber() <= 0)
		{
			cout << "Error, product demand must be positive" << endl;
			exit(1);
		}
		if (product->getWidth() > materialWidth)
		{
			cout << "Error, product width exceeds material width" << endl;
			exit(1);
		}
	}
}

void Controller::loadMaterials(string dataDir)
{
	Json::Reader reader;
	Json::Value root;
	string paramDataFile = dataDir + "materials.json";
	ifstream readData(paramDataFile);

	if (!reader.parse(readData, root, false))
	{
		cout << "Error, materials.json could not be open" << endl;
		exit(1);
	}

	if (_problem == nullptr)
	{
		_problem = new Problem();
		_ownsProblem = true;
	}
	clearMaterials(false);
	vector<PaperRoll* > materials;
	for (int i = 0; i < static_cast<int>(root.size()); i++)
	{
		Json::Value entire_value = root[i];
		int capacity = entire_value["capacity"].asInt();
		int width = entire_value["width"].asInt();
		if (capacity <= 0 || width <= 0)
		{
			cout << "Error, material capacity and width must be positive" << endl;
			exit(1);
		}

		PaperRoll* paperRoll = new PaperRoll(width, capacity, true);
		materials.push_back(paperRoll);
	}
	_problem->setMaterials(materials);
	_ownsMaterials = true;
	resetSolvers();
	syncProblemToSolvers();
}

void Controller::loadProducts(string dataDir)
{
	Json::Reader reader;
	Json::Value root;
	string paramDataFile = dataDir + "products.json";
	ifstream readData(paramDataFile);

	if (!reader.parse(readData, root, false))
	{
		cout << "Error, products.json could not be open" << endl;
		exit(1);
	}

	if (_problem == nullptr)
	{
		_problem = new Problem();
		_ownsProblem = true;
	}
	clearProducts(false);
	vector<PaperRoll* > products;
	for (int i = 0; i < static_cast<int>(root.size()); i++)
	{
		Json::Value entire_value = root[i];
		int demand = entire_value["demand"].asInt();
		int width = entire_value["width"].asInt();
		if (demand <= 0 || width <= 0)
		{
			cout << "Error, product demand and width must be positive" << endl;
			exit(1);
		}

		PaperRoll* paperRoll = new PaperRoll(width, demand, false);
		products.push_back(paperRoll);
	}
	_problem->setProducts(products);
	_ownsProducts = true;
	resetSolvers();
	syncProblemToSolvers();
}

void Controller::solveCG()
{
	validateProblemReady();
	clearPatterns();
	resetSolvers();
	syncProblemToSolvers();

	_masterProblem->initialize();
	_subproblem->initialize();
	
	vector<Pattern* > initialPatterns = findInitialPatterns();
	_patterns.insert(_patterns.end(), initialPatterns.begin(), initialPatterns.end());
	_masterProblem->addColumns(initialPatterns);

	int iter = 0;

	while (true)
	{
		cout << "--------------------------------------------- " << endl;
		cout << "Iteration " << iter << endl;

		_masterProblem->solve();
		_masterProblem->report();
		vector<double> duals = _masterProblem->getDuals();

		_subproblem->setObjective(duals);
		_subproblem->solve();
		_subproblem->report();

		if (_subproblem->getReducedCost() > -Utility::RC_EPS) break;

		Pattern* newPattern = _subproblem->getPattern();
		_patterns.push_back(newPattern);
		_masterProblem->addColumn(newPattern);

		iter++;
	}
}

vector<Pattern* > Controller::findInitialPatterns()
{
	validateProblemReady();

	vector<PaperRoll* > materials = _problem->getMaterials();
	vector<PaperRoll* > products = _problem->getProducts();
	int materialWidth = materials[0]->getWidth();
	int numProduct = static_cast<int>(products.size());

	vector<Pattern* > result;
	for (int i = 0; i < numProduct; i++)
	{
		// create pattern
		PaperRoll* paperRoll = products[i];
		Pattern* newPattern = new Pattern();
		newPattern->addContent(make_pair(i, materialWidth / paperRoll->getWidth()));
		result.push_back(newPattern);
	}

	return result;
}

void Controller::solveIP()
{
	if (_patterns.empty())
	{
		cout << "Error, solveCG must generate columns before solveIP" << endl;
		exit(1);
	}
	_masterProblem->solveIP();
	_masterProblem->reportIP();
}
