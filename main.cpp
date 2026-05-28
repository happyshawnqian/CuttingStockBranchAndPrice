#include <iostream>
#include "Controller.h"
using namespace std;

int main(int argc, char** argv)
{
	Controller controller;

	string inputDataDir = "";
	if (argc > 1) inputDataDir = argv[1];

	controller.loadProducts(inputDataDir);
	controller.loadMaterials(inputDataDir);

	controller.solveCG();
	controller.solveIP();

	system("pause");
}