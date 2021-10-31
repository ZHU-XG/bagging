/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */


#include "Calculations.hpp"
#include "Utils.hpp"



using std::tuple;
using std::pair;
using std::forward_as_tuple;
using std::vector;
using std::string;
using std::unordered_map;

tuple<const Data, const Data> Calculations::partition(const Data& data, const Question& q) {
  Data true_rows; 
  Data false_rows;
  
  for (const auto &row: data) {
    if (q.solve(row))
      true_rows.push_back(row); 
    else
      false_rows.push_back(row);
  }

  return forward_as_tuple(true_rows, false_rows); 
}

/*find the best split among all features and feature values
 * use three threads:
 * first thread iterates cols 0-3
 * second thread iterates cols 4-7
 * third thread iterates cols 8-12
 * */
tuple<const double, const Question> Calculations::find_best_split(const Data& rows, const MetaData& meta) {
    auto best_question = Question();  //keep track of the feature / value that produced it
    std::string best_value;
    int best_feature;
    double best_gain;
    double* best_gain_1 = new double(0.0);  // keep track of the best information gain
    std::string* best_value_1 = new std::string;
    int* best_feature_1 = new int;
    /*first thread*/
    std::thread best_gain_th1([&meta, &rows, best_gain_1, best_value_1, best_feature_1]{
        std::string temp_value_1;
        double temp_ig_1;
        for (int col = 0; col < 3; col++) {  // iterate all columns
            if (meta.type_of_labels.at(col) == "o") {  // ordinal value
                temp_value_1 = std::get<0>(determine_best_threshold_numeric(rows, col));
                temp_ig_1 = std::get<1>(determine_best_threshold_numeric(rows, col));
            }
            else {
                temp_value_1 = std::get<0>(determine_best_threshold_cat(rows, col));
                temp_ig_1 = std::get<1>(determine_best_threshold_cat(rows, col));
            }
            if (temp_ig_1 > *best_gain_1) {
                *best_gain_1 = temp_ig_1;
                *best_feature_1 = col;
                *best_value_1 = temp_value_1;
            }
        }
    });
    /*second thread*/
    double* best_gain_2 = new double(0.0);  // keep track of the best information gain
    std::string* best_value_2 = new std::string;
    int* best_feature_2 = new int;
    std::thread best_gain_th2([&meta, &rows, best_gain_2, best_value_2, best_feature_2]{
        std::string temp_value_2;
        double temp_ig_2;
        for (int col = 4; col < 7; col++) {  // iterate all columns
            if (meta.type_of_labels.at(col) == "o") {  // ordinal value
                temp_value_2 = std::get<0>(determine_best_threshold_numeric(rows, col));
                temp_ig_2 = std::get<1>(determine_best_threshold_numeric(rows, col));
            }
            else {
                temp_value_2 = std::get<0>(determine_best_threshold_cat(rows, col));
                temp_ig_2 = std::get<1>(determine_best_threshold_cat(rows, col));
            }
            if (temp_ig_2 > *best_gain_2) {
                *best_gain_2 = temp_ig_2;
                *best_feature_2 = col;
                *best_value_2 = temp_value_2;
            }
        }
    });
    /*third thread*/
    double* best_gain_3 = new double(0.0);  // keep track of the best information gain
    std::string* best_value_3 = new std::string;
    int* best_feature_3 = new int;
    std::thread best_gain_th3([&meta, &rows, best_gain_3, best_value_3, best_feature_3]{
        std::string temp_value_3;
        double temp_ig_3;
        for (int col = 8; col < 12; col++) {  // iterate all columns
            if (meta.type_of_labels.at(col) == "o") {  // ordinal value
                temp_value_3 = std::get<0>(determine_best_threshold_numeric(rows, col));
                temp_ig_3 = std::get<1>(determine_best_threshold_numeric(rows, col));
            }
            else {
                temp_value_3 = std::get<0>(determine_best_threshold_cat(rows, col));
                temp_ig_3 = std::get<1>(determine_best_threshold_cat(rows, col));
            }
            if (temp_ig_3 > *best_gain_3) {
                *best_gain_3 = temp_ig_3;
                *best_feature_3 = col;
                *best_value_3 = temp_value_3;
            }
        }
    });

    best_gain_th1.join();
    best_gain_th2.join();
    best_gain_th3.join();
    if (*best_gain_1 > *best_gain_2) {
        best_gain = *best_gain_1;
        best_value = *best_value_1;
        best_feature = *best_feature_1;
    } else {
        best_gain = *best_gain_2;
        best_value = *best_value_2;
        best_feature = *best_feature_2;
    }
    delete best_gain_1;
    delete best_gain_2;
    delete best_feature_1;
    delete best_feature_2;
    delete best_value_1;
    delete best_value_2;
    if (*best_gain_3 > best_gain) {
        best_gain = *best_gain_3;
        best_value = *best_value_3;
        best_feature = *best_feature_3;
    }
    delete best_gain_3;
    delete best_feature_3;
    delete best_value_3;
    best_question = Question(best_feature, best_value);
    return forward_as_tuple(best_gain, best_question);
}

/*compute gini index, given class counts and the dataset size;
 * Gini(S) = 1 - sum(pi^2)
 * */
const double Calculations::gini(const ClassCounter& counts, double N) {
    double impurity = 1.0;
    for (const auto & count : counts) {
        impurity -= (double(count.second) / N) * (double(count.second) / N);
    }
    return impurity;
}

/* find the best split value for a discrete ordinal feature;
 * use map and reverse iterating the map to avoid presort, and let initial s1 = empty, and s2 = all decisions;
 * when iterating all values, adding corresponding decisions to s1 and subtracting those decisions from s2;
 * so s1 saves >= v decisions and s2 saves < v decisions.
 * */
tuple<std::string, double> Calculations::determine_best_threshold_numeric(const Data& data, int col) {
    double best_loss = std::numeric_limits<double>::infinity();
    double current_loss;
    std::string best_thresh;
    double data_size = data.size();
    tuple<ClassCounter, NumericClassCounter>* class_counts = new tuple<ClassCounter, NumericClassCounter>;
    *class_counts = numericClassCounts(data, col);
    NumericClassCounter results_of_feature = std::get<1>(*class_counts);  // all decisions in one feature value
    ClassCounter s1; // accumulated values
    ClassCounter s2 = std::get<0>(*class_counts); // inverse accumulated values
    delete class_counts;
    ClassCounter temp_decisions;
    double gini_s = gini(s2, data_size);  // gini of results
    double gini_s1;
    double gini_s2;
    double gain_gini;
    int num_accumulated_decisions = 0;

    for (auto rit = results_of_feature.rbegin(); rit != results_of_feature.rend(); rit++) {  // iterate and find all possible values in one feature
        temp_decisions = rit->second; // corresponding decisions of each value
        for (auto & temp_decision : temp_decisions) {  // iterate decisions of each value
            num_accumulated_decisions += temp_decision.second;  // find summation of all iterated decisions
            s1[temp_decision.first] += temp_decision.second;
            s2.at(temp_decision.first) -= temp_decision.second;
        }
        gini_s1 = gini(s1, num_accumulated_decisions);
        gini_s2 = gini(s2, (data_size - num_accumulated_decisions));
        current_loss = ((num_accumulated_decisions) * gini_s1 + (data_size - num_accumulated_decisions) * gini_s2 ) / data_size;
        if (current_loss < best_loss) {
            best_loss = current_loss;
            best_thresh = std::to_string(rit->first);
        }
    }
    gain_gini = gini_s - best_loss ;
    return forward_as_tuple(best_thresh, gain_gini);
}

/*find the best split value for a categorical feature
 * iterator and set the corresponding value as s1 and residual values are s2
 * so s1 = v, s2 != v.
 **/
tuple<std::string, double> Calculations::determine_best_threshold_cat(const Data& data, int col) {
    double best_loss = std::numeric_limits<double>::infinity();
    double current_loss;
    std::string best_thresh;
    double data_size = data.size();
//    tuple<ClassCounter, FeatureDecisionCounter> class_counts = classCounts(data, col);
    tuple<ClassCounter, FeatureDecisionCounter>* class_counts = new tuple<ClassCounter, FeatureDecisionCounter>;
    *class_counts = catClassCounts(data, col);
    ClassCounter results = std::get<0>(*class_counts);  // all decisions
    FeatureDecisionCounter results_of_feature = std::get<1>(*class_counts);  // decisions in one value
    delete class_counts;
    double gini_s = gini(results, data_size);  // gini of results
    double gini_s1;
    double gini_s2;
    double gain_gini;
    double num_decisions_per_value = 0.0;
  
    for (auto & fit : results_of_feature) {  // iterate all possible values in one feature
        ClassCounter s1 = fit.second; // corresponding decisions of each value
        ClassCounter s2 = results; // residual values
        for (auto & cit : s1) {  // iterate decisions of one value
            num_decisions_per_value += double(cit.second);  // find summation of all iterated decisions
            s2.at(cit.first) -= cit.second;
        }
        gini_s1 = gini(s1, num_decisions_per_value);
        gini_s2 = gini(s2, (data_size - num_decisions_per_value));
        current_loss = (num_decisions_per_value / data_size) * gini_s1 + ((data_size - num_decisions_per_value) / data_size) * gini_s2;
        num_decisions_per_value = 0.0;
        if (current_loss < best_loss) {
            best_loss = current_loss;
            best_thresh = fit.first;
        }
    }
    gain_gini = gini_s - best_loss;
    return forward_as_tuple(best_thresh, gain_gini);
}

/*class counter for categorical features*/
tuple<ClassCounter, FeatureDecisionCounter> Calculations::catClassCounts(const Data& data, int col) {
    ClassCounter counter;
    FeatureDecisionCounter counter_of_feature_decision;
    for (const auto& rows : data) {
        const string decision = *std::rbegin(rows); // the last in one row is its decision
        const string feature = rows.at(col); // the col-th value in one row is what we want
        if (counter.find(decision) != std::end(counter)) {
            counter.at(decision)++;
        }
        else {
            counter[decision] += 1;
        }
        if (counter_of_feature_decision.find(feature) != std::end(counter_of_feature_decision)) {
            counter_of_feature_decision.at(feature)[decision]++; // store decisions of one value in col-th column
        }
        else {
            counter_of_feature_decision[feature][decision] += 1;
        }
    }
    return forward_as_tuple(counter, counter_of_feature_decision);
}

/*class counter for numerical features*/
tuple<ClassCounter, NumericClassCounter> Calculations::numericClassCounts(const Data& data, int col) {
    ClassCounter counter;
    NumericClassCounter counter_of_feature_decision;
    for (const auto& rows : data) {
        const string decision = *std::rbegin(rows); // the last in one row is its decision
        const string feature = rows.at(col); // the col-th value in one row is what we want
        if (counter.find(decision) != std::end(counter)) {
            counter.at(decision)++;
        }
        else {
            counter[decision] += 1;
        }
        if (counter_of_feature_decision.find(std::stoi(feature)) != std::end(counter_of_feature_decision)) {
            counter_of_feature_decision.at(std::stoi(feature))[decision]++; // store decisions of one value in col-th column
        }
        else {
            counter_of_feature_decision[std::stoi(feature)][decision] += 1;
        }
    }
    return forward_as_tuple(counter, counter_of_feature_decision);
}

ClassCounter Calculations::classCounts(const Data &data) {
    ClassCounter counter;
    for (const auto &rows: data) {
        const string decision = *std::rbegin(rows);
        if (counter.find(decision) != std::end(counter)) {
            counter.at(decision)++;
        } else {
            counter[decision] += 1;
        }
    }
    return counter;
}