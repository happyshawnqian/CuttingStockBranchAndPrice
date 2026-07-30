#include <iostream>
#include "Controller.h"
using namespace std;

int main(int argc, char** argv)
{
	Controller controller;

	// Optional first argument is a directory prefix that contains
	// materials.json and products.json. An empty prefix reads from the current
	// working directory.
	string inputDataDir = "";
	if (argc > 1) inputDataDir = argv[1];

	controller.loadProducts(inputDataDir);
	controller.loadMaterials(inputDataDir);

	// Default run path: solve the cutting-stock instance by branch-and-price.
	// solveCG() and solveIP() remain available for comparison experiments.
	controller.solveBP();

	//system("pause");
}
