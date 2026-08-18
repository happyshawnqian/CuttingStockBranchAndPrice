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
	_sequence = 0;
	_depth = 0;
	_poolColumnCount = 0;
	_exactColumnCount = 0;
	_exactSolveCount = 0;
	_hasExactPricingCertificate = false;
}

BPNode::BPNode(int depth)
{
	_id = _counter;
	_counter++;
	_objective = 0;
	_sequence = 0;
	_depth = depth;
	_poolColumnCount = 0;
	_exactColumnCount = 0;
	_exactSolveCount = 0;
	_hasExactPricingCertificate = false;
}

void BPNode::addRyanFosterConstraint(int firstItemIndex, int secondItemIndex,
	RyanFosterBranchType branchType)
{
	if (firstItemIndex == secondItemIndex)
	{
		cout << "Error, a Ryan-Foster constraint requires two different items" << endl;
		exit(1);
	}
	if (firstItemIndex > secondItemIndex)
	{
		// Canonical ordering makes duplicate and conflicting decisions detectable
		// regardless of the order in which callers provide the item indices.
		int temporaryIndex = firstItemIndex;
		firstItemIndex = secondItemIndex;
		secondItemIndex = temporaryIndex;
	}
	for (auto constraint : _ryanFosterConstraints)
	{
		if (constraint.firstItemIndex == firstItemIndex
			&& constraint.secondItemIndex == secondItemIndex)
		{
			if (constraint.branchType == branchType)
			{
				return;
			}
			cout << "Error, conflicting Ryan-Foster decisions for the same item pair" << endl;
			exit(1);
		}
	}

	RyanFosterConstraint constraint;
	constraint.firstItemIndex = firstItemIndex;
	constraint.secondItemIndex = secondItemIndex;
	constraint.branchType = branchType;
	_ryanFosterConstraints.push_back(constraint);
	_values.clear();
	_objective = 0;
	_sequence = 0;
	_poolColumnCount = 0;
	_exactColumnCount = 0;
	_exactSolveCount = 0;
	_hasExactPricingCertificate = false;
}

void BPNode::setActivePatternIndices(const vector<int>& activePatternIndices)
{
	_activePatternIndices = activePatternIndices;
	_values.clear();
	_objective = 0;
	_sequence = 0;
	_poolColumnCount = 0;
	_exactColumnCount = 0;
	_exactSolveCount = 0;
	_hasExactPricingCertificate = false;
}

void BPNode::setExactEvaluation(const vector<int>& activePatternIndices,
	const vector<double>& values, double objective, int sequence,
	int poolColumnCount, int exactColumnCount, int exactSolveCount)
{
	if (activePatternIndices.size() != values.size())
	{
		cout << "Error, active pattern indices and master values have different sizes"
			<< endl;
		exit(1);
	}

	_activePatternIndices = activePatternIndices;
	_values = values;
	_objective = objective;
	_sequence = sequence;
	_poolColumnCount = poolColumnCount;
	_exactColumnCount = exactColumnCount;
	_exactSolveCount = exactSolveCount;
	_hasExactPricingCertificate = true;
}

const vector<BPNode::RyanFosterConstraint>& BPNode::getRyanFosterConstraints() const
{
	return _ryanFosterConstraints;
}

const vector<int>& BPNode::getActivePatternIndices() const
{
	return _activePatternIndices;
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

int BPNode::getPoolColumnCount() const
{
	return _poolColumnCount;
}

int BPNode::getExactColumnCount() const
{
	return _exactColumnCount;
}

int BPNode::getExactSolveCount() const
{
	return _exactSolveCount;
}

bool BPNode::hasExactPricingCertificate() const
{
	return _hasExactPricingCertificate;
}

void BPNode::setDepth(int depth)
{
	_depth = depth;
	_values.clear();
	_objective = 0;
	_sequence = 0;
	_poolColumnCount = 0;
	_exactColumnCount = 0;
	_exactSolveCount = 0;
	_hasExactPricingCertificate = false;
}

bool BPNodeCompare::operator()(const BPNode& left, const BPNode& right) const
{
	if (!left.hasExactPricingCertificate()
		|| !right.hasExactPricingCertificate())
	{
		cout << "Error, best-first comparison requires exact-priced nodes" << endl;
		exit(1);
	}

	// Use an exact ordering here so priority_queue::top() is the node with the
	// true smallest LP objective. Numerical tolerances belong in bound tests,
	// not in a comparator that must provide a strict weak ordering.
	if (left.getObjective() > right.getObjective())
	{
		return true;
	}
	if (left.getObjective() < right.getObjective())
	{
		return false;
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
	_bestNodeId = -1;
	_bestNodeDepth = -1;
	_lowerBound = 0;
	_terminatedByIntegerBound = false;
	_stoppedAtNodeLimit = false;
	_searchTreeExhausted = false;
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
	_lowerBound = 0;
	_terminatedByIntegerBound = false;
	_stoppedAtNodeLimit = false;
	_searchTreeExhausted = false;
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
	_patternRepository.clear();
}

void Controller::validateProblemReady()
{
	// The algorithms assume one material width and at least one product. Failing
	// early here prevents invalid master rows and pricing capacity constraints.
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
		if (product->getNumber() != 1)
		{
			cout << "Error, every product must be a unit-demand item; "
				<< "split aggregate demand before solving" << endl;
			exit(1);
		}
		if (product->getWidth() > materialWidth)
		{
			cout << "Error, product width exceeds material width" << endl;
			exit(1);
		}
	}
}

bool Controller::patternContainsItem(Pattern* pattern, int itemIndex) const
{
	if (pattern == nullptr)
	{
		return false;
	}

	for (auto content : pattern->getContent())
	{
		if (content.first == itemIndex && content.second == 1)
		{
			return true;
		}
	}
	return false;
}

bool Controller::isPatternCompatibleWithNode(const BPNode& node, Pattern* pattern) const
{
	// Apply the branch path to an already generated column. TOGETHER rejects an
	// exclusive-or selection; SEPARATE rejects simultaneous selection.
	for (auto constraint : node.getRyanFosterConstraints())
	{
		bool containsFirst = patternContainsItem(pattern, constraint.firstItemIndex);
		bool containsSecond = patternContainsItem(pattern, constraint.secondItemIndex);

		if (constraint.branchType == BPNode::RyanFosterBranchType::TOGETHER
			&& containsFirst != containsSecond)
		{
			return false;
		}
		if (constraint.branchType == BPNode::RyanFosterBranchType::SEPARATE
			&& containsFirst && containsSecond)
		{
			return false;
		}
	}
	return true;
}

void Controller::applyRyanFosterConstraints(const BPNode& node, Subproblem& subproblem) const
{
	// Pricing must describe the same pattern set as the node master. Otherwise a
	// newly priced column could violate a branch decision enforced on old columns.
	for (auto constraint : node.getRyanFosterConstraints())
	{
		if (constraint.branchType == BPNode::RyanFosterBranchType::TOGETHER)
		{
			subproblem.addTogetherConstraint(
				constraint.firstItemIndex, constraint.secondItemIndex);
		}
		else
		{
			subproblem.addSeparateConstraint(
				constraint.firstItemIndex, constraint.secondItemIndex);
		}
	}
}

string Controller::getItemDescription(int itemIndex) const
{
	ostringstream description;
	description << "item " << itemIndex;

	vector<PaperRoll* > products = _problem->getProducts();
	if (itemIndex >= 0 && itemIndex < static_cast<int>(products.size()))
	{
		PaperRoll* product = products[itemIndex];
		description << " [source " << product->getSourceProductIndex()
			<< ", copy " << product->getCopyIndex()
			<< ", width " << product->getWidth() << "]";
	}
	return description.str();
}

double Controller::getPatternReducedCost(Pattern* pattern,
	const vector<double>& duals) const
{
	if (pattern == nullptr
		|| duals.size() != _problem->getProducts().size())
	{
		cout << "Error, cannot compute pool reduced cost with invalid data" << endl;
		exit(1);
	}

	double reducedCost = pattern->getCost();
	for (auto content : pattern->getContent())
	{
		if (content.first < 0 || content.first >= static_cast<int>(duals.size())
			|| content.second != 1)
		{
			cout << "Error, invalid pattern content during pool pricing" << endl;
			exit(1);
		}
		reducedCost -= duals[content.first];
	}
	return reducedCost;
}

void Controller::addActiveColumnToMaster(int repositoryIndex,
	MasterProblem& master, vector<int>& localToGlobalPatternIndices,
	unordered_set<int>& activePatternIndices) const
{
	if (activePatternIndices.find(repositoryIndex) != activePatternIndices.end())
	{
		cout << "Error, duplicate active global pattern index "
			<< repositoryIndex << endl;
		exit(1);
	}

	Pattern* pattern = _patternRepository.getPattern(repositoryIndex);
	master.addColumn(pattern);
	localToGlobalPatternIndices.push_back(repositoryIndex);
	activePatternIndices.insert(repositoryIndex);
}

int Controller::addNegativePoolColumns(const BPNode& node,
	const vector<double>& duals, MasterProblem& master,
	vector<int>& localToGlobalPatternIndices,
	unordered_set<int>& activePatternIndices) const
{
	int addedColumnCount = 0;
	int repositorySize = _patternRepository.size();
	for (int repositoryIndex = 0;
		repositoryIndex < repositorySize; repositoryIndex++)
	{
		if (activePatternIndices.find(repositoryIndex) != activePatternIndices.end())
		{
			continue;
		}

		Pattern* pattern = _patternRepository.getPattern(repositoryIndex);
		if (!isPatternCompatibleWithNode(node, pattern))
		{
			continue;
		}

		if (getPatternReducedCost(pattern, duals) < -Utility::RC_EPS)
		{
			addActiveColumnToMaster(repositoryIndex, master,
				localToGlobalPatternIndices, activePatternIndices);
			addedColumnCount++;
		}
	}
	return addedColumnCount;
}

void Controller::validateActivePatternIndices(const BPNode& node,
	const vector<int>& activePatternIndices) const
{
	unordered_set<int> uniqueIndices;
	for (auto repositoryIndex : activePatternIndices)
	{
		if (uniqueIndices.find(repositoryIndex) != uniqueIndices.end())
		{
			cout << "Error, node id " << node.getId()
				<< " contains duplicate active pattern index "
				<< repositoryIndex << endl;
			exit(1);
		}

		Pattern* pattern = _patternRepository.getPattern(repositoryIndex);
		if (!isPatternCompatibleWithNode(node, pattern))
		{
			cout << "Error, node id " << node.getId()
				<< " contains a Ryan-Foster-incompatible active pattern index "
				<< repositoryIndex << endl;
			exit(1);
		}
		uniqueIndices.insert(repositoryIndex);
	}
}

void Controller::requireExactPricingCertificate(const BPNode& node) const
{
	if (!node.hasExactPricingCertificate())
	{
		cout << "Error, node id " << node.getId()
			<< " has no exact-pricing lower-bound certificate" << endl;
		exit(1);
	}
}

bool Controller::solveColumnGenerationAtNode(BPNode& node)
{
	// The local master contains only this node's active repository patterns.
	// Pool pricing activates known columns before exact pricing searches the
	// complete Ryan-Foster-compatible pattern space.
	MasterProblem master;
	master.setMaterials(_problem->getMaterials());
	master.setProducts(_problem->getProducts());
	master.initialize();
	master.addArtificialColumns(1000000);

	validateActivePatternIndices(node, node.getActivePatternIndices());
	vector<int> localToGlobalPatternIndices;
	unordered_set<int> activePatternIndices;
	for (auto repositoryIndex : node.getActivePatternIndices())
	{
		addActiveColumnToMaster(repositoryIndex, master,
			localToGlobalPatternIndices, activePatternIndices);
	}

	Subproblem subproblem;
	subproblem.setMaterials(_problem->getMaterials());
	subproblem.setProducts(_problem->getProducts());
	subproblem.initialize();
	applyRyanFosterConstraints(node, subproblem);

	int poolColumnCount = 0;
	int exactColumnCount = 0;
	int exactSolveCount = 0;

	while (true)
	{
		if (!master.solve())
		{
			return false;
		}

		vector<double> duals = master.getDuals();
		int poolColumnsAdded = addNegativePoolColumns(node, duals, master,
			localToGlobalPatternIndices, activePatternIndices);
		if (poolColumnsAdded > 0)
		{
			poolColumnCount += poolColumnsAdded;
			continue;
		}

		subproblem.setObjective(duals);
		exactSolveCount++;
		if (!subproblem.solve(true))
		{
			return false;
		}

		double exactReducedCost = subproblem.getReducedCost();
		if (exactReducedCost >= -Utility::RC_EPS)
		{
			break;
		}

		Pattern* newPattern = subproblem.getPattern();
		if (!isPatternCompatibleWithNode(node, newPattern))
		{
			cout << "Error, pricing generated a pattern that violates the current "
				<< "Ryan-Foster constraints" << endl;
			delete newPattern;
			exit(1);
		}

#ifdef DEBUG
		cout << "************* new pattern *************" << endl;
		newPattern->print();
		cout << "reduced cost = " << exactReducedCost << endl;
#endif

		PatternRepository::AddResult addResult =
			_patternRepository.addOrGet(newPattern);
		if (activePatternIndices.find(addResult.patternIndex)
			!= activePatternIndices.end())
		{
			Pattern* activePattern =
				_patternRepository.getPattern(addResult.patternIndex);
			cout << "Error, exact pricing returned active global pattern index "
				<< addResult.patternIndex << " (Pattern id "
				<< activePattern->getId() << ") with reduced cost "
				<< exactReducedCost << ", pool reduced cost "
				<< getPatternReducedCost(activePattern, duals) << endl;
			exit(1);
		}

		addActiveColumnToMaster(addResult.patternIndex, master,
			localToGlobalPatternIndices, activePatternIndices);
		if (addResult.inserted)
		{
			exactColumnCount++;
		}
	}

	vector<double> values = master.getValues();
	if (values.size() != localToGlobalPatternIndices.size())
	{
		cout << "Error, local master values do not match the local-to-global "
			<< "pattern mapping" << endl;
		exit(1);
	}

	validateActivePatternIndices(node, localToGlobalPatternIndices);
	if (master.getArtificialUsage() > Utility::RC_EPS)
	{
		// Exact pricing has exhausted the full compatible pattern space, so
		// positive artificial usage certifies that the real node is infeasible.
		return false;
	}

	node.setExactEvaluation(localToGlobalPatternIndices, values,
		master.getObjectiveValue(), _nextBranchAndPriceSequence,
		poolColumnCount, exactColumnCount, exactSolveCount);
	_nextBranchAndPriceSequence++;
	return true;
}

bool Controller::isIntegerPatternSolution(const vector<double>& values) const
{
	for (auto value : values)
	{
		double roundedValue = floor(value + 0.5);
		if (fabs(value - roundedValue) > Utility::RC_EPS)
		{
			return false;
		}
	}
	return true;
}

bool Controller::findRyanFosterPair(const BPNode& node, RyanFosterPair& pair) const
{
	requireExactPricingCertificate(node);
	const vector<double>& values = node.getValues();
	const vector<int>& activePatternIndices = node.getActivePatternIndices();
	int itemCount = static_cast<int>(_problem->getProducts().size());
	int patternCount = static_cast<int>(values.size());
	if (patternCount != static_cast<int>(activePatternIndices.size()))
	{
		cout << "Error, Ryan-Foster branching received an invalid node column mapping"
			<< endl;
		exit(1);
	}

	// Aggregate y_ij over the current LP solution. Because each item is covered
	// exactly once, y_ij lies in [0, 1] and measures how strongly i and j are
	// assigned to the same selected patterns.
	vector<vector<double>> togetherValues(
		itemCount, vector<double>(itemCount, 0));
	for (int patternIndex = 0; patternIndex < patternCount; patternIndex++)
	{
		double patternValue = values[patternIndex];
		if (patternValue <= Utility::RC_EPS)
		{
			continue;
		}

		vector<int> patternItems;
		int repositoryIndex = activePatternIndices[patternIndex];
		Pattern* pattern = _patternRepository.getPattern(repositoryIndex);
		for (auto content : pattern->getContent())
		{
			if (content.second == 1 && content.first >= 0 && content.first < itemCount)
			{
				patternItems.push_back(content.first);
			}
		}

		for (int firstPosition = 0;
			firstPosition < static_cast<int>(patternItems.size()); firstPosition++)
		{
			for (int secondPosition = firstPosition + 1;
				secondPosition < static_cast<int>(patternItems.size()); secondPosition++)
			{
				int firstItemIndex = patternItems[firstPosition];
				int secondItemIndex = patternItems[secondPosition];
				if (firstItemIndex > secondItemIndex)
				{
					int temporaryIndex = firstItemIndex;
					firstItemIndex = secondItemIndex;
					secondItemIndex = temporaryIndex;
				}
				togetherValues[firstItemIndex][secondItemIndex] += patternValue;
			}
		}
	}

	// Values near zero or one already satisfy a Ryan-Foster disjunction. Among
	// fractional pairs, choose the value closest to 0.5 to balance the children.
	bool pairFound = false;
	double bestDistance = 1.0e100;
	for (int firstItemIndex = 0; firstItemIndex < itemCount; firstItemIndex++)
	{
		for (int secondItemIndex = firstItemIndex + 1;
			secondItemIndex < itemCount; secondItemIndex++)
		{
			double togetherValue = togetherValues[firstItemIndex][secondItemIndex];
			if (togetherValue <= Utility::RC_EPS
				|| togetherValue >= 1 - Utility::RC_EPS)
			{
				continue;
			}

			double distance = fabs(togetherValue - 0.5);
			if (!pairFound || distance < bestDistance - Utility::RC_EPS)
			{
				pair.firstItemIndex = firstItemIndex;
				pair.secondItemIndex = secondItemIndex;
				pair.togetherValue = togetherValue;
				bestDistance = distance;
				pairFound = true;
			}
		}
	}
	return pairFound;
}

bool Controller::evaluateBPNode(BPNode& node)
{
	if (_processedBranchAndPriceNodes >= _maxBranchAndPriceNodes)
	{
		return false;
	}

	_processedBranchAndPriceNodes++;

	if (!solveColumnGenerationAtNode(node))
	{
		return false;
	}
	requireExactPricingCertificate(node);
	const vector<double>& values = node.getValues();
	double objective = node.getObjective();

	cout << "Node id " << node.getId() << ", depth " << node.getDepth() << ", processed " << _processedBranchAndPriceNodes
		<< ", LP objective = " << objective
		<< ", active columns = " << node.getActivePatternIndices().size()
		<< ", pool activations = " << node.getPoolColumnCount()
		<< ", exact generated = " << node.getExactColumnCount()
		<< ", exact solves = " << node.getExactSolveCount()
		<< ", exact pricing = complete" << endl;

	if (objective >= _bestObjective - Utility::RC_EPS)
	{
		return false;
	}

	if (isIntegerPatternSolution(values))
	{
		_bestObjective = objective;
		_bestSolution.clear();
		const vector<int>& activePatternIndices = node.getActivePatternIndices();
		for (int localIndex = 0;
			localIndex < static_cast<int>(values.size()); localIndex++)
		{
			int quantity = static_cast<int>(floor(values[localIndex] + 0.5));
			if (quantity > 0)
			{
				PatternUsage usage;
				usage.repositoryIndex = activePatternIndices[localIndex];
				usage.quantity = quantity;
				_bestSolution.push_back(usage);
			}
		}
		_bestNodeId = node.getId();
		_bestNodeDepth = node.getDepth();
		cout << "New incumbent uses " << _bestObjective << " rolls at node id "
			<< node.getId() << " on level " << node.getDepth() << endl;
		return false;
	}

	return true;
}

void Controller::createRyanFosterChildNodes(const BPNode& node, const RyanFosterPair& pair,
	BPNode& togetherNode, BPNode& separateNode)
{
	requireExactPricingCertificate(node);
	// Both children inherit the complete branch path. Adding complementary
	// decisions for the selected pair partitions the parent's feasible patterns:
	// the together child allows both/neither, and the separate child forbids both.
	// Each child also inherits every parent active column that remains compatible.
	togetherNode.setDepth(node.getDepth() + 1);
	for (auto constraint : node.getRyanFosterConstraints())
	{
		togetherNode.addRyanFosterConstraint(constraint.firstItemIndex,
			constraint.secondItemIndex, constraint.branchType);
	}
	togetherNode.addRyanFosterConstraint(pair.firstItemIndex, pair.secondItemIndex,
		BPNode::RyanFosterBranchType::TOGETHER);

	separateNode.setDepth(node.getDepth() + 1);
	for (auto constraint : node.getRyanFosterConstraints())
	{
		separateNode.addRyanFosterConstraint(constraint.firstItemIndex,
			constraint.secondItemIndex, constraint.branchType);
	}
	separateNode.addRyanFosterConstraint(pair.firstItemIndex, pair.secondItemIndex,
		BPNode::RyanFosterBranchType::SEPARATE);

	vector<int> togetherActivePatternIndices;
	vector<int> separateActivePatternIndices;
	for (auto repositoryIndex : node.getActivePatternIndices())
	{
		Pattern* pattern = _patternRepository.getPattern(repositoryIndex);
		if (isPatternCompatibleWithNode(togetherNode, pattern))
		{
			togetherActivePatternIndices.push_back(repositoryIndex);
		}
		if (isPatternCompatibleWithNode(separateNode, pattern))
		{
			separateActivePatternIndices.push_back(repositoryIndex);
		}
	}
	togetherNode.setActivePatternIndices(togetherActivePatternIndices);
	separateNode.setActivePatternIndices(separateActivePatternIndices);
}

void Controller::reportRyanFosterBranch(const BPNode& node, const RyanFosterPair& pair,
	const BPNode& togetherNode, const BPNode& separateNode) const
{
	cout << "Ryan-Foster branch at node id " << node.getId() << ": "
		<< getItemDescription(pair.firstItemIndex) << " and "
		<< getItemDescription(pair.secondItemIndex)
		<< ", together value = " << pair.togetherValue << endl;
	cout << "  Together child node id " << togetherNode.getId()
		<< ", separate child node id " << separateNode.getId() << endl;
}

bool Controller::hasBranchAndPriceIncumbent() const
{
	return _bestNodeId >= 0;
}

bool Controller::isIntegerBoundClosed() const
{
	if (!hasBranchAndPriceIncumbent())
	{
		return false;
	}

	// Under the current unit-cost model, every feasible integer objective is a
	// whole number of rolls. Therefore, ceil(LP lower bound) is a valid integer
	// lower bound.
	double integerLowerBound = ceil(_lowerBound - Utility::BP_BOUND_EPS);
	return integerLowerBound >= _bestObjective - Utility::BP_BOUND_EPS;
}

void Controller::reportBranchAndPriceBounds() const
{
	double integerLowerBound = ceil(_lowerBound - Utility::BP_BOUND_EPS);
	cout << "Global LP lower bound = " << _lowerBound
		<< ", integer lower bound = " << integerLowerBound;
	if (hasBranchAndPriceIncumbent())
	{
		cout << ", upper bound = " << _bestObjective;
	}
	else
	{
		cout << ", upper bound = not available";
	}
	cout << endl;
}

void Controller::solveBranchAndPriceNode(const BPNode& node)
{
	// Best-first branch-and-price. Each open node is stored with its LP bound,
	// and the node with the smallest bound is branched first.
	priority_queue<BPNode, vector<BPNode>, BPNodeCompare> openNodes;

	BPNode rootNode = node;
	if (evaluateBPNode(rootNode))
	{
		requireExactPricingCertificate(rootNode);
		openNodes.push(rootNode);
	}

	while (!openNodes.empty())
	{
		// All nodes in the queue have completed pricing, so the smallest queued
		// LP objective is the global lower bound for every unexplored subtree.
		requireExactPricingCertificate(openNodes.top());
		_lowerBound = openNodes.top().getObjective();
		reportBranchAndPriceBounds();

		if (isIntegerBoundClosed())
		{
			_terminatedByIntegerBound = true;
			break;
		}

		// Expanding one fractional node requires solving both children. Keep the
		// current node in the frontier if the remaining node budget is too small;
		// its LP objective then remains a valid lower bound for that subtree.
		if (_processedBranchAndPriceNodes + 2 > _maxBranchAndPriceNodes)
		{
			_stoppedAtNodeLimit = true;
			break;
		}

		BPNode current = openNodes.top();
		openNodes.pop();
		requireExactPricingCertificate(current);

		if (current.getObjective() >= _bestObjective - Utility::RC_EPS)
		{
			continue;
		}

		RyanFosterPair branchPair;
		if (!findRyanFosterPair(current, branchPair))
		{
			// For a fractional set-partitioning solution, Ryan-Foster theory
			// guarantees at least one pair with a fractional together value.
			cout << "Error, node id " << current.getId()
				<< " has fractional pattern variables but no fractional "
				<< "Ryan-Foster item pair" << endl;
			exit(1);
		}

		BPNode togetherNode;
		BPNode separateNode;
		createRyanFosterChildNodes(
			current, branchPair, togetherNode, separateNode);
		reportRyanFosterBranch(
			current, branchPair, togetherNode, separateNode);

		if (evaluateBPNode(togetherNode))
		{
			requireExactPricingCertificate(togetherNode);
			openNodes.push(togetherNode);
		}

		if (evaluateBPNode(separateNode))
		{
			requireExactPricingCertificate(separateNode);
			openNodes.push(separateNode);
		}
	}

	if (!_terminatedByIntegerBound && openNodes.empty())
	{
		_searchTreeExhausted = true;
		if (hasBranchAndPriceIncumbent())
		{
			_lowerBound = _bestObjective;
		}
	}
}

void Controller::reportBranchAndPriceSolution()
{
	// Incumbent entries retain repository indices, so later repository growth
	// cannot change which Pattern each positive integer quantity references.
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
		for (auto usage : _bestSolution)
		{
			Pattern* pattern =
				_patternRepository.getPattern(usage.repositoryIndex);
			cout << "  Global pattern index " << usage.repositoryIndex
				<< ", Pattern " << pattern->getId()
				<< " = " << usage.quantity << endl;
			pattern->print();
		}
	}
	cout << "Global LP lower bound = " << _lowerBound << endl;
	cout << "Global integer lower bound = "
		<< ceil(_lowerBound - Utility::BP_BOUND_EPS) << endl;
	if (hasBranchAndPriceIncumbent())
	{
		cout << "Global upper bound = " << _bestObjective << endl;
	}
	cout << "Processed " << _processedBranchAndPriceNodes << " branch-and-price nodes" << endl;
	cout << "Global repository contains " << _patternRepository.size()
		<< " unique patterns" << endl;
	if (_terminatedByIntegerBound)
	{
		cout << "Branching stopped because the integer lower bound reached the incumbent" << endl;
	}
	else if (_stoppedAtNodeLimit)
	{
		cout << "Warning, branch-and-price stopped at the node limit" << endl;
	}
	else if (_searchTreeExhausted && hasBranchAndPriceIncumbent())
	{
		cout << "Optimality proven after exhausting the branch-and-price tree" << endl;
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
	//   width  - item width
	//   demand - number of identical items to create
	// Each created PaperRoll is a distinct unit-demand item. Keeping copies
	// distinct is required by the binary set-partitioning formulation and by
	// Ryan-Foster branching on pairs of unit-demand items.
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

		for (int copyIndex = 0; copyIndex < demand; copyIndex++)
		{
			PaperRoll* paperRoll = new PaperRoll(width, 1, false);
			paperRoll->setSourceProductIndex(i);
			paperRoll->setCopyIndex(copyIndex);
			products.push_back(paperRoll);
		}
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
	_patternRepository.reset(
		static_cast<int>(_problem->getProducts().size()));
	resetSolvers();
	syncProblemToSolvers();

	_masterProblem->initialize();
	_subproblem->initialize();

	vector<Pattern* > initialPatterns = findInitialPatterns();
	for (auto initialPattern : initialPatterns)
	{
		PatternRepository::AddResult addResult =
			_patternRepository.addOrGet(initialPattern);
		if (!addResult.inserted)
		{
			cout << "Error, duplicate singleton pattern during initialization" << endl;
			exit(1);
		}
		Pattern* repositoryPattern =
			_patternRepository.getPattern(addResult.patternIndex);
		_masterProblem->addColumn(repositoryPattern);
		_subproblem->addExcludedPattern(repositoryPattern);
	}

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
		if (!_subproblem->solve(false))
		{
			break;
		}
		_subproblem->report();

		if (_subproblem->getReducedCost() > -Utility::RC_EPS) break;

		Pattern* newPattern = _subproblem->getPattern();
		PatternRepository::AddResult addResult =
			_patternRepository.addOrGet(newPattern);
		Pattern* repositoryPattern =
			_patternRepository.getPattern(addResult.patternIndex);
		if (!addResult.inserted)
		{
			// This is a defensive check in addition to the binary no-good cuts.
			_subproblem->addExcludedPattern(repositoryPattern);
			continue;
		}
		_masterProblem->addColumn(repositoryPattern);
		_subproblem->addExcludedPattern(repositoryPattern);

		iter++;
	}
}

vector<Pattern* > Controller::findInitialPatterns()
{
	// A basic feasible column set: one singleton pattern per unit-demand item.
	// These columns guarantee a feasible set-partitioning master before pricing
	// starts combining compatible items into lower-cost patterns.
	validateProblemReady();

	vector<PaperRoll* > products = _problem->getProducts();
	int numProduct = static_cast<int>(products.size());

	vector<Pattern* > result;
	for (int i = 0; i < numProduct; i++)
	{
		Pattern* newPattern = new Pattern();
		newPattern->addContent(make_pair(i, 1));
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
	if (_patternRepository.size() == 0)
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
	// Branch-and-price entry point. The repository and root active set start with
	// singleton patterns; node pricing activates known columns or adds new ones.
	validateProblemReady();
	_patternRepository.reset(
		static_cast<int>(_problem->getProducts().size()));
	resetSolvers();
	syncProblemToSolvers();

	_bestObjective = 1.0e100;
	_bestSolution.clear();
	_bestNodeId = -1;
	_bestNodeDepth = -1;
	_lowerBound = 0;
	_terminatedByIntegerBound = false;
	_stoppedAtNodeLimit = false;
	_searchTreeExhausted = false;
	_processedBranchAndPriceNodes = 0;
	_nextBranchAndPriceSequence = 0;

	vector<Pattern* > initialPatterns = findInitialPatterns();
	vector<int> rootActivePatternIndices;
	for (auto pattern : initialPatterns)
	{
		PatternRepository::AddResult addResult =
			_patternRepository.addOrGet(pattern);
		if (!addResult.inserted)
		{
			cout << "Error, duplicate singleton pattern during initialization" << endl;
			exit(1);
		}
		rootActivePatternIndices.push_back(addResult.patternIndex);
	}

	BPNode root(0);
	root.setActivePatternIndices(rootActivePatternIndices);
	solveBranchAndPriceNode(root);
	reportBranchAndPriceSolution();
}
