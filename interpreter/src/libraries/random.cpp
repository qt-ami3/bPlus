using namespace std;
#include <cmath>
#include <random>
#include <utility>
#include "../../include/libraries/random.h"

//  One generator for the whole run. Building an mt19937 from random_device on
//  every call is slow, and where random_device is not a real entropy source it
//  hands back the same seed every time, which shows up as a "random" number
//  that never changes.
static mt19937& generator() {
  static mt19937 rng(random_device{}());
  return rng;
}

void randomint(int& result, int from, int up_to) {
  if (from > up_to) swap(from, up_to);

  uniform_int_distribution<int> d_int(from, up_to);
  result = d_int(generator());
}

void randomdouble(double& result, double from, double up_to) {
  if (from > up_to) swap(from, up_to);

  uniform_real_distribution<double> d_real(from, up_to);
  result = d_real(generator());
}

void randombool(bool& result) {
  bernoulli_distribution d_bool(0.5);
  result = d_bool(generator());
}

void doublebellcurve(double& result, double mean, double deviation) {
  deviation = fabs(deviation);

  if (deviation == 0) {  //  normal_distribution requires a positive spread.
    result = mean;
    return;
  }

  normal_distribution<double> d_norm(mean, deviation);
  result = d_norm(generator());
}
