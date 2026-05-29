#include "Controller.h"
#include <cstdlib>
#include <cmath>
#include <sstream>

Controller::Controller()
{
	_problem = new Problem();
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = true;
	_ownsMaterials = false;
	_ownsProducts = false;
	_bestObjective = 1.0e100;
	_processedBranchAndPriceNodes = 0;
	_maxBranchAndPriceNodes = 1000;
}

Controller::Controller(Problem* problem)
{
	_problem = problem;
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = false;
	_ownsMaterials = false;
	_ownsProducts = false;
	_bestObjective = 1.0e100;
	_processedBranchAndPriceNodes = 0;
	_maxBranchAndPriceNodes = 1000;

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

string Controller::getPatternSignature(Pattern* pattern)
{
	int nWdth = 0;
	if (_problem != nullptr)
	{
		nWdth = static_cast<int>(_problem->getProducts().size());
	}

	vector<int> counts(nWdth, 0);
	if (pattern != nullptr)
	{
		for (auto content : pattern->getContent())
		{
			if (content.first >= 0 && content.first < nWdth)
			{
				counts[content.first] += content.second;
			}
		}
	}

	return getPatternSignature(counts);
}

string Controller::getPatternSignature(const vector<int>& counts)
{
	ostringstream signature;
	for (int i = 0; i < static_cast<int>(counts.size()); i++)
	{
		if (i > 0) signature << ",";
		signature << counts[i];
	}
	return signature.str();
}

bool Controller::isKnownPattern(Pattern* pattern)
{
	return isKnownPatternSignature(getPatternSignature(pattern));
}

bool Controller::isKnownPatternSignature(const string& signature)
{
	for (auto knownPattern : _patterns)
	{
		if (getPatternSignature(knownPattern) == signature)
		{
			return true;
		}
	}
	return false;
}

bool Controller::getPatternBounds(const BranchNode& node, Pattern* pattern, double& lowerBound, double& upperBound)
{
	string signature = getPatternSignature(pattern);
	int lower = 0;
	int upper = -1;

	for (auto bound : node.bounds)
	{
		if (bound.signature == signature)
		{
			if (bound.lowerBound > lower)
			{
				lower = bound.lowerBound;
			}
			if (bound.upperBound >= 0 && (upper < 0 || bound.upperBound < upper))
			{
				upper = bound.upperBound;
			}
		}
	}

	if (upper >= 0 && lower > upper)
	{
		return false;
	}

	lowerBound = lower;
	upperBound = upper >= 0 ? upper : IloInfinity;
	return true;
}

void Controller::addBranchBound(BranchNode& node, const string& signature, int lowerBound, int upperBound)
{
	PatternBound bound;
	bound.signature = signature;
	bound.lowerBound = lowerBound;
	bound.upperBound = upperBound;
	node.bounds.push_back(bound);
}

void Controller::enumeratePricingPatterns(int productIndex, int remainingWidth, const vector<double>& duals,
	vector<int>& counts, double& bestReducedCost, vector<int>& bestCounts)
{
	vector<PaperRoll* > products = _problem->getProducts();
	if (productIndex == static_cast<int>(products.size()))
	{
		string signature = getPatternSignature(counts);
		if (isKnownPatternSignature(signature))
		{
			return;
		}

		double reducedCost = _problem->getMaterials()[0]->getCost();
		for (int i = 0; i < static_cast<int>(counts.size()); i++)
		{
			reducedCost -= duals[i] * counts[i];
		}

		if (reducedCost < bestReducedCost)
		{
			bestReducedCost = reducedCost;
			bestCounts = counts;
		}
		return;
	}

	int width = products[productIndex]->getWidth();
	int maxUse = remainingWidth / width;
	for (int amount = 0; amount <= maxUse; amount++)
	{
		counts[productIndex] = amount;
		enumeratePricingPatterns(productIndex + 1, remainingWidth - amount * width,
			duals, counts, bestReducedCost, bestCounts);
	}
	counts[productIndex] = 0;
}

Pattern* Controller::findBestPricingPattern(const vector<double>& duals)
{
	vector<PaperRoll* > materials = _problem->getMaterials();
	vector<PaperRoll* > products = _problem->getProducts();
	vector<int> counts(products.size(), 0);
	vector<int> bestCounts(products.size(), 0);
	double bestReducedCost = 1.0e100;

	enumeratePricingPatterns(0, materials[0]->getWidth(), duals, counts, bestReducedCost, bestCounts);
	if (bestReducedCost > -Utility::RC_EPS)
	{
		return nullptr;
	}

	Pattern* pattern = new Pattern();
	vector<pair<int, int>> content;
	for (int i = 0; i < static_cast<int>(bestCounts.size()); i++)
	{
		if (bestCounts[i] > 0)
		{
			content.push_back(make_pair(i, bestCounts[i]));
		}
	}
	pattern->setContent(content);
	pattern->setCost(materials[0]->getCost());
	return pattern;
}

bool Controller::solveColumnGenerationAtNode(const BranchNode& node, vector<double>& values, double& objective)
{
	MasterProblem master;
	master.setMaterials(_problem->getMaterials());
	master.setProducts(_problem->getProducts());
	master.initialize();
	master.addArtificialColumns(1000000);

	for (auto pattern : _patterns)
	{
		double lowerBound = 0;
		double upperBound = IloInfinity;
		if (!getPatternBounds(node, pattern, lowerBound, upperBound))
		{
			return false;
		}
		master.addColumn(pattern, lowerBound, upperBound);
	}

	while (true)
	{
		if (!master.solve())
		{
			return false;
		}

		vector<double> duals = master.getDuals();
		Pattern* newPattern = findBestPricingPattern(duals);
		if (newPattern == nullptr) break;

		_patterns.push_back(newPattern);
		double lowerBound = 0;
		double upperBound = IloInfinity;
		if (!getPatternBounds(node, newPattern, lowerBound, upperBound))
		{
			return false;
		}
		master.addColumn(newPattern, lowerBound, upperBound);
	}

	values = master.getValues();
	objective = master.getObjectiveValue();
	if (master.getArtificialUsage() > Utility::RC_EPS)
	{
		return false;
	}

	return true;
}

int Controller::findFractionalPatternIndex(const vector<double>& values)
{
	for (int i = 0; i < static_cast<int>(values.size()); i++)
	{
		double rounded = floor(values[i] + 0.5);
		if (fabs(values[i] - rounded) > Utility::RC_EPS)
		{
			return i;
		}
	}
	return -1;
}

void Controller::solveBranchAndPriceNode(const BranchNode& node)
{
	if (_processedBranchAndPriceNodes >= _maxBranchAndPriceNodes)
	{
		return;
	}
	_processedBranchAndPriceNodes++;

	vector<double> values;
	double objective = 0;
	if (!solveColumnGenerationAtNode(node, values, objective))
	{
		return;
	}
	cout << "Node " << _processedBranchAndPriceNodes << ", depth " << node.depth
		<< ", LP objective = " << objective << endl;
	if (objective >= _bestObjective - Utility::RC_EPS)
	{
		return;
	}

	int branchIndex = findFractionalPatternIndex(values);
	if (branchIndex < 0)
	{
		_bestObjective = objective;
		_bestSolution = values;
		cout << "New incumbent uses " << _bestObjective << " rolls at node "
			<< _processedBranchAndPriceNodes << endl;
		return;
	}

	double value = values[branchIndex];
	int floorValue = static_cast<int>(floor(value));
	int ceilValue = floorValue + 1;
	string signature = getPatternSignature(_patterns[branchIndex]);

	BranchNode downBranch = node;
	downBranch.depth = node.depth + 1;
	addBranchBound(downBranch, signature, 0, floorValue);

	BranchNode upBranch = node;
	upBranch.depth = node.depth + 1;
	addBranchBound(upBranch, signature, ceilValue, -1);

	solveBranchAndPriceNode(upBranch);
	solveBranchAndPriceNode(downBranch);
}

void Controller::reportBranchAndPriceSolution()
{
	cout << endl;
	if (_bestSolution.empty())
	{
		cout << "No integer solution found by branch-and-price" << endl;
	}
	else
	{
		cout << "Best branch-and-price solution uses " << _bestObjective << " rolls" << endl;
		for (int i = 0; i < static_cast<int>(_bestSolution.size()) && i < static_cast<int>(_patterns.size()); i++)
		{
			int value = static_cast<int>(floor(_bestSolution[i] + 0.5));
			if (value > 0)
			{
				cout << "  Pattern " << _patterns[i]->getId() << " = " << value << endl;
			}
		}
	}
	cout << "Processed " << _processedBranchAndPriceNodes << " branch-and-price nodes" << endl;
	if (_processedBranchAndPriceNodes >= _maxBranchAndPriceNodes)
	{
		cout << "Warning, branch-and-price stopped at the node limit" << endl;
	}
	cout << endl;
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

		if (!_masterProblem->solve())
		{
			exit(1);
		}
		_masterProblem->report();
		vector<double> duals = _masterProblem->getDuals();

		_subproblem->setObjective(duals);
		if (!_subproblem->solve())
		{
			exit(1);
		}
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
	if (!_masterProblem->solveIP())
	{
		exit(1);
	}
	_masterProblem->reportIP();
}

void Controller::solveBP()
{
	validateProblemReady();
	clearPatterns();
	resetSolvers();
	syncProblemToSolvers();

	_bestObjective = 1.0e100;
	_bestSolution.clear();
	_processedBranchAndPriceNodes = 0;

	vector<Pattern* > initialPatterns = findInitialPatterns();
	for (auto pattern : initialPatterns)
	{
		if (isKnownPattern(pattern))
		{
			delete pattern;
		}
		else
		{
			_patterns.push_back(pattern);
		}
	}

	BranchNode root;
	root.depth = 0;
	solveBranchAndPriceNode(root);
	reportBranchAndPriceSolution();
}
