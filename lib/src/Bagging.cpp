/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#include "Bagging.hpp"

using std::make_shared;
using std::shared_ptr;
using std::string;
using boost::timer::cpu_timer;

Bagging::Bagging(const DataReader& dr, const int ensembleSize, uint seed) : 
  dr_(dr), 
  ensembleSize_(ensembleSize),
  learners_({}) {
  random_number_generator.seed(seed);
  buildBag();
}


void Bagging::buildBag() {
  cpu_timer timer;
  std::vector<double> timings;
  const MetaData meta = dr_.metaData();
  for (int i = 0; i < ensembleSize_; i++) {
      Data* bootstrapped_data = new Data;
      while ((*bootstrapped_data).size() < (dr_.trainData()).size() ){
          int random_row_index = int(random_number_generator() % (dr_.trainData().size() - 1));
          bootstrapped_data->push_back(dr_.trainData()[random_row_index]);
      } // randomly choose rows from data
      timer.start();
      DecisionTree dt(dr_, *bootstrapped_data);
      delete bootstrapped_data;
      auto nanoseconds = boost::chrono::nanoseconds(timer.elapsed().wall);
      auto seconds = boost::chrono::duration_cast<boost::chrono::seconds>(nanoseconds);
      timings.push_back(seconds.count());
      learners_.push_back(dt);
  }
  float avg_timing = Utils::iterators::average(std::begin(timings), std::begin(timings) + std::min(5, ensembleSize_));
  std::cout << "Average timing: " << avg_timing << std::endl;
}

void Bagging::test() const {
  TreeTest t;
  float accuracy = 0;
  for (const auto& row: dr_.testData()) {
    static size_t last = row.size() - 1;
    std::vector<std::string> decisions;
    for (int i = 0; i < ensembleSize_; i++) {
      const std::shared_ptr<Node> root = std::make_shared<Node>(learners_.at(i).root_);
      const auto& classification = t.classify(row, root);
      decisions.push_back(Utils::tree::getMax(classification));
    }
    std::string prediction = Utils::iterators::mostCommon(decisions.begin(), decisions.end());
    if (prediction == row[last])
      accuracy += 1;
  }
  std::cout << "Total accuracy: " << (accuracy / dr_.testData().size()) << std::endl;
}


