/*
 * Copyright (c) DTAI - KU Leuven – All rights reserved.
 * Proprietary, do not copy or distribute without permission. 
 * Written by Pieter Robberechts, 2019
 */

#include "DecisionTree.hpp"
#include "set"
#include "future"

using std::tuple;
using std::make_shared;
using std::shared_ptr;
using std::string;
using boost::timer::cpu_timer;

DecisionTree::DecisionTree(const DataReader& dr) : root_(Node()), dr_(dr) {
    std::cout << "Start building tree." << std::endl; cpu_timer timer;
    root_ = buildTree(dr_.trainData(), dr_.metaData());
    std::cout << "Done. " << timer.format() << std::endl;
}

DecisionTree::DecisionTree(const DataReader& dr, Data& data) : root_(Node()), dr_(dr) {
    std::cout << "Start building tree." << std::endl; cpu_timer timer;
    root_ = buildTree(data, dr_.metaData());
    std::cout << "Done. " << timer.format() << std::endl;
}

const Node DecisionTree::buildTree(const Data& rows, const MetaData& meta) {
    tuple<const double, const Question> gain_question = Calculations::find_best_split(rows, meta);
    double gain = std::get<0>(gain_question);
    Question question = std::get<1>(gain_question);

    if (gain < 10e-6) {
        Leaf leaf(Calculations::classCounts(rows));
        Node leafNode(leaf);
        return leafNode;
    }
    else {
        tuple<const Data, const Data> left_right = Calculations::partition(rows, question);
        Node* right_branch = new Node;
        Node* left_branch = new Node;
        std::thread branching_right([this, &left_right, &meta, right_branch]() {
            *right_branch = buildTree(std::get<0>(left_right), meta);
        });

        std::thread branching_left([this, &left_right, &meta, left_branch]() {
            *left_branch = buildTree(std::get<1>(left_right), meta);
        });

        branching_right.join();
        branching_left.join();
        Node new_node = Node(*right_branch, *left_branch, question);
        delete right_branch;
        delete left_branch;
        return new_node;
    }
}

void DecisionTree::print() const {
  print(make_shared<Node>(root_));
}

void DecisionTree::print(const shared_ptr<Node> root, string spacing) const {
  if (bool is_leaf = root->leaf() != nullptr; is_leaf) {
    const auto &leaf = root->leaf();
    std::cout << spacing + "Predict: "; Utils::print::print_map(leaf->predictions());
    return;
  }
  std::cout << spacing << root->question().toString(dr_.metaData().labels) << "\n";

  std::cout << spacing << "--> True: " << "\n";
  print(root->trueBranch(), spacing + "   ");

  std::cout << spacing << "--> False: " << "\n";
  print(root->falseBranch(), spacing + "   ");
}

void DecisionTree::test() const {
  TreeTest t(dr_.testData(), dr_.metaData(), root_);
}
