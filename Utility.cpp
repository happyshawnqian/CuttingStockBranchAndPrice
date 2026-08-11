#include "Utility.h"

// Reduced-cost tolerance shared by master/pricing logic.
double Utility::RC_EPS = 1.0e-6;

// Bound tolerance for the integer optimality test in branch-and-price.
const double Utility::BP_BOUND_EPS = 1.0e-6;
