#include "Controller.h"
#include <cstdlib>
#include <cmath>
#include <queue>
#include <sstream>

int BPNode::_counter = 0;

BPNode::BPNode()
{
	_id = _counter;
	_counter++;
	_objective = 0;
	_patternCount = 0;
	_sequence = 0;
	_depth = 0;
	_evaluated = false;
}

BPNode::BPNode(int depth)
{
	_id = _counter;
	_counter++;
	_objective = 0;
	_patternCount = 0;
	_sequence = 0;
	_depth = depth;
	_evaluated = false;
}

void BPNode::addBound(const string& signature, int lowerBound, int upperBound)
{
	PatternBound bound;
	bound.signature = signature;
	bound.lowerBound = lowerBound;
	bound.upperBound = upperBound;
	_bounds.push_back(bound);
}

void BPNode::setEvaluation(const vector<double>& values, double objective, int patternCount, int sequence)
{
	_values = values;
	_objective = objective;
	_patternCount = patternCount;
	_sequence = sequence;
	_evaluated = true;
}

bool BPNode::isEvaluated() const
{
	return _evaluated;
}

bool BPNode::isSolvedWithPatternCount(int patternCount) const
{
	return _evaluated && _patternCount == patternCount;
}

const vector<BPNode::PatternBound>& BPNode::getBounds() const
{
	return _bounds;
}

const vector<double>& BPNode::getValues() const
{
	return _values;
}

int BPNode::getId() const
{
	return _id;
}

double BPNode::getObjective() const
{
	return _objective;
}

int BPNode::getDepth() const
{
	return _depth;
}

int BPNode::getSequence() const
{
	return _sequence;
}

void BPNode::setDepth(int depth)
{
	_depth = depth;
	_values.clear();
	_objective = 0;
	_patternCount = 0;
	_sequence = 0;
	_evaluated = false;
}

bool BPNodeCompare::operator()(const BPNode& left, const BPNode& right) const
{
	if (fabs(left.getObjective() - right.getObjective()) > Utility::RC_EPS)
	{
		return left.getObjective() > right.getObjective();
	}
	if (left.getDepth() != right.getDepth())
	{
		return left.getDepth() > right.getDepth();
	}
	return left.getSequence() > right.getSequence();
}

Controller::Controller()
{
	// Default construction creates an empty problem that this controller owns.
	// Data loaded from JSON will be stored in this Problem instance.
	_problem = new Problem();
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = true;
	_ownsMaterials = false;
	_ownsProducts = false;
	_bestObjective = 1.0e100;
	_processedBranchAndPriceNodes = 0;
	_maxBranchAndPriceNodes = 1000;
	_nextBranchAndPriceSequence = 0;
}

Controller::Controller(Problem* problem)
{
	// External problems are borrowed. The controller synchronizes solver views
	// with the supplied data but does not delete the Problem or its rolls.
	_problem = problem;
	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
	_ownsProblem = false;
	_ownsMaterials = false;
	_ownsProducts = false;
	_bestObjective = 1.0e100;
	_bestNodeId = -1;
	_bestNodeDepth = -1;
	_processedBranchAndPriceNodes = 0;
	_maxBranchAndPriceNodes = 1000;
	_nextBranchAndPriceSequence = 0;

	syncProblemToSolvers();
}

Controller::~Controller()
{
	// Clear owned column/data objects before destroying the solver wrappers.
	// Solver wrappers only reference Problem data and must not delete it.
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
	// Replacing the problem invalidates any generated columns and CPLEX models,
	// because both are tied to the old product/material vectors.
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
	// Master and pricing models keep their own copies of the pointer vectors.
	// The PaperRoll objects themselves are shared and owned elsewhere.
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
	// Concert models are append-only in this codebase. Rebuilding the solver
	// wrappers avoids stale rows, columns, bounds, and objective expressions.
	delete _masterProblem;
	delete _subproblem;

	_masterProblem = new MasterProblem();
	_subproblem = new Subproblem();
}

void Controller::clearMaterials(bool clearProblemVector)
{
	// clearProblemVector controls whether the Problem should forget the vector.
	// When reloading JSON, the old owned objects are deleted but the Problem
	// remains alive and receives a new vector immediately after this call.
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
	// Products follow the same ownership policy as materials: delete only when
	// they were loaded by this controller, never when supplied externally.
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
	// The algorithms assume one material width and at least one product. Failing
	// early here avoids out-of-range access and division by zero in pricing.
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
	// Convert sparse Pattern content into a dense count vector ordered by
	// product index. This makes logically identical patterns compare equal.
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
	// A comma-separated vector is sufficient here because product counts are
	// non-negative integers and product order is fixed by the input data.
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

bool Controller::getPatternBounds(const BPNode& node, Pattern* pattern, double& lowerBound, double& upperBound)
{
	// A node can inherit multiple bounds for the same pattern. The effective
	// lower bound is the maximum lower bound; the effective upper bound is the
	// minimum finite upper bound.
	string signature = getPatternSignature(pattern);
	int lower = 0;
	int upper = -1;

	for (auto bound : node.getBounds())
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

bool Controller::solveColumnGenerationAtNode(const BPNode& node, vector<double>& values, double& objective)
{
	// Build a fresh restricted master for this branch node. Existing global
	// columns are inserted with the node-specific bounds inherited from the
	// branch path, then pricing adds new columns until no negative reduced-cost
	// pattern remains.
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

	Subproblem subproblem;
	subproblem.setMaterials(_problem->getMaterials());
	subproblem.setProducts(_problem->getProducts());
	subproblem.initialize();
	subproblem.addExcludedPatterns(_patterns);

	while (true)
	{
		// Solve the current LP relaxation, then let the pricing subproblem find
		// a pattern with the most negative reduced cost.
		if (!master.solve())
		{
			return false;
		}

		vector<double> duals = master.getDuals();
		subproblem.setObjective(duals);
		if (!subproblem.solve())
		{
			break;
		}
		if (subproblem.getReducedCost() > -Utility::RC_EPS)
		{
			break;
		}

		Pattern* newPattern = subproblem.getPattern();

#ifdef DEBUG
		cout << "************* new pattern *************" << endl;
		newPattern->print();
		cout << "reduced cost = " << subproblem.getReducedCost() << endl;
#endif

		if (isKnownPattern(newPattern))
		{
			// A duplicate column must not be added under a new Pattern id,
			// otherwise branch bounds on this pattern signature could be bypassed.
			subproblem.addExcludedPattern(newPattern);
			delete newPattern;
			continue;
		}

		_patterns.push_back(newPattern);
		double lowerBound = 0;
		double upperBound = IloInfinity;
		if (!getPatternBounds(node, newPattern, lowerBound, upperBound))
		{
			return false;
		}
		master.addColumn(newPattern, lowerBound, upperBound);
		subproblem.addExcludedPattern(newPattern);
	}

	values = master.getValues();
	objective = master.getObjectiveValue();
	if (master.getArtificialUsage() > Utility::RC_EPS)
	{
		// Artificial usage means real generated columns cannot satisfy this
		// branch node's demand and bounds, so the node is infeasible.
		return false;
	}

	return true;
}

int Controller::findFractionalPatternIndex(const vector<double>& values)
{
	// Branch on the first fractional master variable. This is a simple pattern
	// variable branching rule; stronger rules such as Ryan-Foster are possible.
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

bool Controller::evaluateBPNode(BPNode& node)
{
	if (_processedBranchAndPriceNodes >= _maxBranchAndPriceNodes)
	{
		return false;
	}

	_processedBranchAndPriceNodes++;

	vector<double> values;
	double objective = 0;
	if (!solveColumnGenerationAtNode(node, values, objective))
	{
		return false;
	}

	cout << "Node id " << node.getId() << ", depth " << node.getDepth() << ", processed " << _processedBranchAndPriceNodes
		<< ", LP objective = " << objective << endl;

	if (objective >= _bestObjective - Utility::RC_EPS)
	{
		return false;
	}

	if (findFractionalPatternIndex(values) < 0)
	{
		_bestObjective = objective;
		_bestSolution = values;
		_bestNodeId = node.getId();
		_bestNodeDepth = node.getDepth();
		cout << "New incumbent uses " << _bestObjective << " rolls at node id "
			<< node.getId() << " on level " << node.getDepth() << endl;
		return false;
	}

	node.setEvaluation(values, objective, static_cast<int>(_patterns.size()), _nextBranchAndPriceSequence);
	_nextBranchAndPriceSequence++;
	return true;
}

void Controller::createChildNodes(const BPNode& node, int branchIndex, BPNode& downNode, BPNode& upNode)
{
	double value = node.getValues()[branchIndex];
	int floorValue = static_cast<int>(floor(value));
	int ceilValue = floorValue + 1;
	string signature = getPatternSignature(_patterns[branchIndex]);

	// Split the fractional variable x_p = value into:
	//   down: x_p <= floor(value)
	//   up:   x_p >= ceil(value)
	downNode.setDepth(node.getDepth() + 1);
	for (auto bound : node.getBounds())
	{
		downNode.addBound(bound.signature, bound.lowerBound, bound.upperBound);
	}
	downNode.addBound(signature, 0, floorValue);

	upNode.setDepth(node.getDepth() + 1);
	for (auto bound : node.getBounds())
	{
		upNode.addBound(bound.signature, bound.lowerBound, bound.upperBound);
	}
	upNode.addBound(signature, ceilValue, -1);
}

void Controller::solveBranchAndPriceNode(const BPNode& node)
{
	// Best-first branch-and-price. Each open node is stored with its LP bound,
	// and the node with the smallest bound is branched first.
	priority_queue<BPNode, vector<BPNode>, BPNodeCompare> openNodes;

	BPNode rootNode = node;
	if (evaluateBPNode(rootNode))
	{
		openNodes.push(rootNode);
	}

	while (!openNodes.empty() && _processedBranchAndPriceNodes < _maxBranchAndPriceNodes)
	{
		BPNode current = openNodes.top();
		openNodes.pop();

		// The global column pool can grow while other nodes are evaluated. If
		// this node was solved with an older pool, refresh its LP bound before
		// using it for branching.
		//if (!current.isSolvedWithPatternCount(static_cast<int>(_patterns.size())))
		//{
		//	if (evaluateBPNode(current))
		//	{
		//		openNodes.push(current);
		//	}
		//	continue;
		//}

		if (current.getObjective() >= _bestObjective - Utility::RC_EPS)
		{
			continue;
		}

		int branchIndex = findFractionalPatternIndex(current.getValues());
		if (branchIndex < 0)
		{
			_bestObjective = current.getObjective();
			_bestSolution = current.getValues();
			_bestNodeId = current.getId();
			_bestNodeDepth = current.getId();
			cout << "New incumbent uses " << _bestObjective << " rolls at node id "
				<< current.getId() << " on level " << current.getDepth() << endl;
			continue;
		}

		BPNode upNode;
		BPNode downNode;
		createChildNodes(current, branchIndex, downNode, upNode);

		if (evaluateBPNode(upNode))
		{
			openNodes.push(upNode);
		}

		if (evaluateBPNode(downNode))
		{
			openNodes.push(downNode);
		}
	}
}

void Controller::reportBranchAndPriceSolution()
{
	// Report only positive pattern variables from the incumbent. Pattern IDs
	// are stable enough for output but signatures should be used for logic.
	cout << endl;
	if (_bestSolution.empty())
	{
		cout << "No integer solution found by branch-and-price" << endl;
	}
	else
	{
		cout << "Best branch-and-price solution uses " << _bestObjective
			<< " rolls, found at node id " << _bestNodeId
			<< " on level " << _bestNodeDepth << endl;
		for (int i = 0; i < static_cast<int>(_bestSolution.size()) && i < static_cast<int>(_patterns.size()); i++)
		{
			int value = static_cast<int>(floor(_bestSolution[i] + 0.5));
			if (value > 0)
			{
				cout << "  Pattern " << _patterns[i]->getId() << " = " << value << endl;
				_patterns[i]->print();
			}
		}
	}
	cout << "Processed " << _processedBranchAndPriceNodes << " branch-and-price nodes" << endl;
	cout << "Generated " << _patterns.size() << " patterns" << endl;
	if (_processedBranchAndPriceNodes >= _maxBranchAndPriceNodes)
	{
		cout << "Warning, branch-and-price stopped at the node limit" << endl;
	}
	cout << endl;
}

void Controller::loadMaterials(string dataDir)
{
	// Input path is a directory prefix. For example, passing "data\\" reads
	// "data\\materials.json"; passing an empty string reads from the cwd.
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
	// Product JSON fields:
	//   width  - demanded product width
	//   demand - required number of pieces
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
	// Standalone column generation: solve the LP relaxation only, using the
	// CPLEX Subproblem pricing model. This path is kept for comparison with
	// the branch-and-price implementation.
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
	// A basic feasible column set: one pattern per product, each pattern cutting
	// as many pieces of that product as fit into one raw roll.
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

#ifdef DEBUG
		cout << "********** initial pattern **********" << endl;
		newPattern->print();
#endif
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
	// Branch-and-price entry point. The root column pool starts with simple
	// single-product patterns; each node can add more patterns through pricing.
	validateProblemReady();
	clearPatterns();
	resetSolvers();
	syncProblemToSolvers();

	_bestObjective = 1.0e100;
	_bestSolution.clear();
	_bestNodeId = -1;
	_bestNodeDepth = -1;
	_processedBranchAndPriceNodes = 0;
	_nextBranchAndPriceSequence = 0;

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

	BPNode root(0);
	solveBranchAndPriceNode(root);
	reportBranchAndPriceSolution();
}
