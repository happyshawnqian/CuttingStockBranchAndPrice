#pragma once
//#define DEBUG

class Utility
{
public:
	// Numerical tolerance for reduced-cost checks and integer/fractional tests.
	static double RC_EPS;

	// Numerical tolerance used when converting the branch-and-price LP lower
	// bound into an integer lower bound on the number of rolls.
	static const double BP_BOUND_EPS;
};
