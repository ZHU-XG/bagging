/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#include "../lib/include/Bagging.hpp"

int main() {
  Dataset d;
  d.train.filename = "G:/code/assignment/code/test/fruit.arff";
  d.test.filename = "G:/code/assignment/code/test/fruit_test.arff";

  Bagging bc(d, 5);
  bc.test();
  return 0;
}
