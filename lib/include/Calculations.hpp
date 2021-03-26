/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#ifndef DECISIONTREE_CALCULATIONS_HPP
#define DECISIONTREE_CALCULATIONS_HPP

#include <tuple>
#include <vector>
#include <string>
#include <unordered_map>
#include <boost/timer/timer.hpp>
#include "Question.hpp"
#include "Utils.hpp"

using ClassCounter = std::unordered_map<std::string, int>;
using FeatureDecisionCounter = std::unordered_map<std::string, ClassCounter>;

namespace Calculations {

std::tuple<const Data, const Data> partition(const Data &data, const Question &q);

const double gini(const ClassCounter& counts, double N);

std::tuple<const double, const Question> find_best_split(Data &rows, const MetaData &meta);

std::tuple<std::string, double> determine_best_threshold_numeric(Data &data, int col);

std::tuple<std::string, double> determine_best_threshold_cat(Data &data, int col);

bool compare_rows(const VecS& v1, const VecS& v2, int col);
void sort_col(Data& data, int col);


bool isNum(string str);

struct ClassCounts {
    ClassCounter ctr;
    FeatureDecisionCounter ctr_of_feature_decision;
};
} // namespace Calculations

#endif //DECISIONTREE_CALCULATIONS_HPP
