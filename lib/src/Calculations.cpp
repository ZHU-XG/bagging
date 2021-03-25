/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#include <cmath>
#include <algorithm>
#include <iterator>
#include "Calculations.hpp"
#include "Utils.hpp"
#include "functional"

using std::tuple;
using std::pair;
using std::forward_as_tuple;
using std::vector;
using std::string;
using std::unordered_map;

tuple<const Data, const Data> Calculations::partition(const Data& data, const Question& q) {
  Data true_rows; // 只有一个属性是true，其他都是false
  Data false_rows;
  
  for (const auto &row: data) {
    if (q.solve(row))
      true_rows.push_back(row); 
    else
      false_rows.push_back(row);
  }

  return forward_as_tuple(true_rows, false_rows); 
}

tuple<const double, const Question> Calculations::find_best_split(const Data& rows, const MetaData& meta) {
  double best_gain = 0.0;  // keep track of the best information gain
  auto best_question = Question();  //keep track of the feature / value that produced it
  // TODO: find the best split among all features and feature values
  // data二维，里面是v，外面是class
  // classcounts 后返回class数和相应的个数
  //const ClassCounter ctr = Calculations::classCounts(rows);  // rows是某个属性的二维表格
  //const double gini_s = Calculations::gini(ctr, double(rows.size()));  // 某个属性的所有可能取值



  return forward_as_tuple(best_gain, best_question);
}

const double Calculations::gini(const ClassCounter& counts, double N) {  
  // compute gini index, given class counts and the dataset size;
  // Gini(S) = 1 - sum(pi^2)
  double impurity = 1.0;
  for (ClassCounter::const_iterator cit = counts.begin(); cit != counts.end(); cit++) {
      impurity -= (cit->second / N) * (cit->second / N);
  }
  return impurity;
}

tuple<std::string, double> Calculations::determine_best_threshold_numeric(Data& data, int col) {
  //TODO: find the best split value for a discrete ordinal feature
  sort_col(data, col);
    double best_loss = std::numeric_limits<float>::infinity();
    double current_loss = 0.0;
    std::string best_thresh;
    double data_size = data.size();
    ClassCounts class_counts = classCounts(data, col);
    ClassCounter results = class_counts.ctr;  // 最后decision的统计
    FeatureDecisionCounter results_of_feature = class_counts.ctr_of_feature_decision;  // 一个feature里的decision的统计
    ClassCounter s1; // s1此时为累计值
    ClassCounter s2; // residual values
    double gini_s = gini(results, data_size);  // gini of results
    double gini_s1;
    double gini_s2;
    double gain_gini;
    double num_decisions_per_value = 0.0;

    for (FeatureDecisionCounter::iterator fit = results_of_feature.begin(); fit != results_of_feature.end(); fit++) {  // 迭代feature里所有可能的value
        s1 = fit->second; // 每个value对应的decisions 
        s2 = results;
        for (ClassCounter::iterator cit = s1.begin(); cit != s1.end(); cit++) {  // 遍历每个value对应的decisions
            num_decisions_per_value += double(cit->second);  // 将decision数加起来
            s2.at(cit->first) -= cit->second;
        }
        gini_s1 = gini(s1, num_decisions_per_value);
        gini_s2 = gini(s2, data_size - num_decisions_per_value);
        current_loss = (num_decisions_per_value / data_size) * gini_s1 + ((data_size - num_decisions_per_value) / data_size) * gini_s2;
        num_decisions_per_value = 0.0;
        if (current_loss < best_loss) {
            best_loss = current_loss;
            best_thresh = fit->first;
            if (best_loss == 0.0) {
                break;
            }
        }
    }
    gain_gini = gini_s - best_loss;
    return forward_as_tuple(best_thresh, gain_gini);
}

/*
    find the best split value for a categorical feature
*/
tuple<std::string, double> Calculations::determine_best_threshold_cat(Data& data, int col) {
  sort_col(data, col);
  double best_loss = std::numeric_limits<float>::infinity();
  double current_loss = 0.0;
  std::string best_thresh;
  double data_size = data.size();
  ClassCounts class_counts = classCounts(data, col);
  ClassCounter results = class_counts.ctr;  // 最后decision的统计
  FeatureDecisionCounter results_of_feature = class_counts.ctr_of_feature_decision;  // 一个feature里的decision的统计
  ClassCounter s1;
  ClassCounter s2; // residual values
  double gini_s = gini(results, data_size);  // gini of results
  double gini_s1;
  double gini_s2;
  double gain_gini;
  double num_decisions_per_value = 0.0;
  
  for (FeatureDecisionCounter::iterator fit = results_of_feature.begin(); fit != results_of_feature.end(); fit++) {  // 迭代feature里所有可能的value
      s1 = fit->second; // 每个value对应的decisions 
      s2 = results;
      for (ClassCounter::iterator cit = s1.begin(); cit != s1.end(); cit++) {  // 遍历每个value对应的decisions
          num_decisions_per_value += double(cit->second);  // 将decision数加起来
          s2.at(cit->first) -= cit->second;
      }
      gini_s1 = gini(s1, num_decisions_per_value);
      gini_s2 = gini(s2, data_size - num_decisions_per_value);
      current_loss = (num_decisions_per_value / data_size) * gini_s1 + ((data_size - num_decisions_per_value) / data_size) * gini_s2;
      num_decisions_per_value = 0.0;
      if (current_loss < best_loss) {
          best_loss = current_loss;
          best_thresh = fit->first;
          if (best_loss == 0.0) {
              break;
          }
      }
  }
  gain_gini = gini_s - best_loss;
  return forward_as_tuple(best_thresh, gain_gini);
}

Calculations::ClassCounts classCounts(const Data& data, int col) {  // 输入data
    Calculations::ClassCounts cc;
    ClassCounter counter;
    FeatureDecisionCounter counter_of_feature_decision;
    for (const auto& rows : data) {  // 遍历每一行
        const string decision = *std::rbegin(rows); // 每一行的最后一个为decision
        const string feature = rows.at(col); // 每行第col个为feature
        if (counter.find(decision) != std::end(counter)) {
            counter.at(decision)++;
        }
        else {
            counter[decision] += 1;
        }
        if (counter_of_feature_decision.find(feature) != std::end(counter_of_feature_decision)) { // 存储了某个col对应的取值对应的decision，计算gini时只考虑一个取值
            counter_of_feature_decision.at(feature).at(decision)++;
        }
        else {
            counter_of_feature_decision[feature][decision] += 1;
        }
    }
    cc.ctr = counter;
    cc.ctr_of_feature_decision = counter_of_feature_decision;
    return cc;
}

bool Calculations::compare_rows(const VecS& v1, const VecS& v2, int col) {
    return v1[col] < v2[col];
}

void Calculations::sort_col(Data& data, int col) {
    sort(data.begin(), data.end(), std::bind(compare_rows, std::placeholders::_1, std::placeholders::_2, col));
}