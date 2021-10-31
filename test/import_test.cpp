/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#include <DecisionTree/DecisionTree.hpp>

int main(void) {
  Dataset d;
  d.train.filename = "/home/r0815637/code/test/data/covtype.arff";
  d.test.filename = "/home/r0815637/code/test/data/covtype_test.arff";

  DecisionTree dt(d);
  return 0;
}
