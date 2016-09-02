/*
 * IntAutomaton.cpp
 *
 *  Created on: Jun 24, 2015
 *      Author: baki
 */

#include "IntAutomaton.h"

namespace Vlab {
namespace Theory {

const int IntAutomaton::INFINITE = -2;

const int IntAutomaton::VLOG_LEVEL = 9;

int IntAutomaton::DEFAULT_NUM_OF_VARIABLES = 8;

int* IntAutomaton::DEFAULT_VARIABLE_INDICES = Automaton::getIndices(
        IntAutomaton::DEFAULT_NUM_OF_VARIABLES);

unsigned* IntAutomaton::DEFAULT_UNSIGNED_VARIABLE_INDICES = Automaton::getIndices(
        (unsigned)IntAutomaton::DEFAULT_NUM_OF_VARIABLES);

IntAutomaton::IntAutomaton(DFA_ptr dfa) :
        Automaton(Automaton::Type::INT, dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES),
        has_negative_1(false) {

}
IntAutomaton::IntAutomaton(DFA_ptr dfa, int num_of_variables) :
        Automaton(Automaton::Type::INT, dfa, num_of_variables),
        has_negative_1 (false) {
}

IntAutomaton::IntAutomaton(DFA_ptr dfa, bool has_negative_1) :
        Automaton(Automaton::Type::INT, dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES),
        has_negative_1 (has_negative_1) {
}

IntAutomaton::IntAutomaton(DFA_ptr dfa, bool has_negative_1, int num_of_variables) :
        Automaton(Automaton::Type::INT, dfa, num_of_variables),
        has_negative_1 (has_negative_1) {

}

IntAutomaton::IntAutomaton(const IntAutomaton& other) :
        Automaton(other), has_negative_1 (other.has_negative_1) {
}

IntAutomaton::~IntAutomaton() {
}

IntAutomaton_ptr IntAutomaton::clone() const {
  IntAutomaton_ptr cloned_auto = new IntAutomaton(*this);
  DVLOG(VLOG_LEVEL) << cloned_auto->id << " = [" << this->id << "]->clone()";
  return cloned_auto;
}

void IntAutomaton::release_default_indices() {
  delete[] DEFAULT_VARIABLE_INDICES;
  DEFAULT_VARIABLE_INDICES = nullptr;
  delete[] DEFAULT_UNSIGNED_VARIABLE_INDICES;
  DEFAULT_UNSIGNED_VARIABLE_INDICES = nullptr;
}

IntAutomaton_ptr IntAutomaton::makePhi(int num_of_variables, int* variable_indices) {
  DFA_ptr non_accepting_int_dfa = nullptr;
  IntAutomaton_ptr non_acception_int_auto = nullptr;
  non_accepting_int_dfa = Automaton::makePhi(num_of_variables, variable_indices);
  non_acception_int_auto = new IntAutomaton(non_accepting_int_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << non_acception_int_auto->id << " = makePhi()";

  return non_acception_int_auto;
}

IntAutomaton_ptr IntAutomaton::makeZero(int num_of_variables, int* variable_indices) {
  DFA_ptr zero_int_dfa = nullptr;
  IntAutomaton_ptr zero_int = nullptr;
  char statuses[2] { '+', '-' };

  dfaSetup(2, num_of_variables, variable_indices);
  dfaAllocExceptions(0);
  dfaStoreState(1);
  dfaAllocExceptions(0);
  dfaStoreState(1);
  zero_int_dfa = dfaBuild(statuses);
  zero_int = new IntAutomaton(zero_int_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << zero_int->id << " = makeZero()";

  return zero_int;
}

/**
 *
 * Returns Sigma* except two reserved words (11111111, 11111110)
 */
IntAutomaton_ptr IntAutomaton::makeAnyInt(int num_of_variables, int* variable_indices) {
  DFA_ptr any_int_dfa = nullptr;
  IntAutomaton_ptr any_int = nullptr;
  char statuses[2] { '+', '-' };
  std::vector<char> reserved_1 = Automaton::getReservedWord('1', num_of_variables);
  std::vector<char> reserved_2 = Automaton::getReservedWord('0', num_of_variables);
  char* sharp1 = &*reserved_1.begin();
  char* sharp0 = &*reserved_2.begin();

  dfaSetup(2, num_of_variables, variable_indices);
  dfaAllocExceptions(2);
  dfaStoreException(1, sharp1); // word 11111111
  dfaStoreException(1, sharp0); // word 11111110
  dfaStoreState(0);

  dfaAllocExceptions(0);
  dfaStoreState(1);

  any_int_dfa = dfaBuild(statuses);
  any_int = new IntAutomaton(any_int_dfa, true, num_of_variables);

  DVLOG(VLOG_LEVEL) << any_int->id << " = makeAnyInt()";

  return any_int;
}

IntAutomaton_ptr IntAutomaton::makeInt(int value, int num_of_variables, int* variable_indices){
  DFA_ptr int_dfa = nullptr;
  IntAutomaton_ptr int_auto = nullptr;

  if(value < 0){
    int_auto = IntAutomaton::makePhi();
    int_auto->has_negative_1 = true;
  }
  else if (value == 0){
    int_auto = IntAutomaton::makeZero();
  }
  else{
    LOG(FATAL) << "implement me";
    //    int_dfa = dfaStringAutomatonL1toL2(value, value,
//             IntAutomaton::DEFAULT_NUM_OF_VARIABLES, IntAutomaton::DEFAULT_VARIABLE_INDICES);
//         int_auto = new IntAutomaton(int_dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES);
  }

  DVLOG(VLOG_LEVEL) << int_auto->id << " = makeInt(" << value <<  ")";

  return int_auto;
}

IntAutomaton_ptr IntAutomaton::makeIntLessThan(int value, int num_of_variables, int* variable_indices){
  DFA_ptr int_dfa = nullptr;
  IntAutomaton_ptr int_auto = nullptr;

   if(value < 0){
     int_auto = IntAutomaton::makePhi();
   }
   else if (value == 0){
     int_auto = IntAutomaton::makePhi();
     int_auto->has_negative_1 = true;
   }
   else{
     LOG(FATAL) << "implement me";
//     int_dfa = dfaStringAutomatonL1toL2(0, value - 1,
//         IntAutomaton::DEFAULT_NUM_OF_VARIABLES, IntAutomaton::DEFAULT_VARIABLE_INDICES);
//     int_auto = new IntAutomaton(int_dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES);
   }

   DVLOG(VLOG_LEVEL) << int_auto->id << " = makeIntLessThan(" << value <<  ")";

   return int_auto;
}

IntAutomaton_ptr IntAutomaton::makeIntLessThanOrEqual(int value, int num_of_variables, int* variable_indices){
  DFA_ptr int_dfa = nullptr;
  IntAutomaton_ptr int_auto = nullptr;

  if(value < 0){
    int_auto = IntAutomaton::makePhi();
    int_auto->has_negative_1 = true;
  }
  else if (value == 0){
    int_auto = IntAutomaton::makeZero();
    int_auto->has_negative_1 = true;
  }
  else{
    LOG(FATAL) << "implement me";
//    int_dfa = dfaStringAutomatonL1toL2(0, value,
//             IntAutomaton::DEFAULT_NUM_OF_VARIABLES, IntAutomaton::DEFAULT_VARIABLE_INDICES);
//         int_auto = new IntAutomaton(int_dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES);
  }

  DVLOG(VLOG_LEVEL) << int_auto->id << " = makeIntLessThanEqual(" << value <<  ")";

  return int_auto;
}

IntAutomaton_ptr IntAutomaton::makeIntGreaterThan(int value, int num_of_variables, int* variable_indices){
  IntAutomaton_ptr int_auto = nullptr;

  if(value < -1){
    int_auto = IntAutomaton::makeAnyInt();
  }
  else{
    IntAutomaton_ptr less_than_or_equal = IntAutomaton::makeIntLessThanOrEqual(value);
    int_auto = less_than_or_equal->complement();
    delete less_than_or_equal;
  }

  DVLOG(VLOG_LEVEL) << int_auto->id << " = makeIntGreaterThan(" << value <<  ")";

  return int_auto;
}

IntAutomaton_ptr IntAutomaton::makeIntGreaterThanOrEqual(int value, int num_of_variables, int* variable_indices){
  IntAutomaton_ptr int_auto = nullptr;

  if(value < -1){
    int_auto = IntAutomaton::makeAnyInt();
  }
  else{
    IntAutomaton_ptr less_auto = IntAutomaton::makeIntLessThan(value);
    int_auto = less_auto->complement();
    delete less_auto;
  }

  DVLOG(VLOG_LEVEL) << int_auto->id << " = makeIntGreaterThanEqual(" << value <<  ")";

  return int_auto;
}

IntAutomaton_ptr IntAutomaton::makeIntRange(int start, int end, int num_of_variables, int* variable_indices){
  DFA_ptr int_dfa = nullptr;
  IntAutomaton_ptr range_auto = nullptr;
  LOG(FATAL) << "implement me";
//  int_dfa = dfaStringAutomatonL1toL2(start, end,
//           IntAutomaton::DEFAULT_NUM_OF_VARIABLES, IntAutomaton::DEFAULT_VARIABLE_INDICES);
//  range_auto = new IntAutomaton(int_dfa, IntAutomaton::DEFAULT_NUM_OF_VARIABLES);

  DVLOG(VLOG_LEVEL) << range_auto->id << " = makeIntRange(" << start << "," << end <<  ")";

  return range_auto;
}

IntAutomaton_ptr IntAutomaton::makeInts(std::vector<int> values, int num_of_variables, int* variable_indices) {
  IntAutomaton_ptr int_auto = nullptr;
  if (values.size() > 0) {
    auto max = std::max_element(values.begin(), values.end());

    int_auto = IntAutomaton::makeInt(*max, num_of_variables, variable_indices);

    for (int i : values) {
      if (i < 0) {
        int_auto->has_negative_1 = true;
      } else {
        int_auto->dfa->f[i] = 1;
      }
    }
  } else {
    int_auto = IntAutomaton::makePhi(num_of_variables, variable_indices);
  }

  DVLOG(VLOG_LEVEL) << int_auto->id << " = makeInts({...})";

  return int_auto;
}

void IntAutomaton::setMinus1(bool has_minus_one) {
  has_negative_1 = has_minus_one;
}

bool IntAutomaton::hasNegative1() {
  return has_negative_1;
}
IntAutomaton_ptr IntAutomaton::complement() {
  DFA_ptr complement_dfa = nullptr, minimized_dfa = nullptr, current_dfa = dfaCopy(dfa);
  IntAutomaton_ptr complement_auto = nullptr;
  IntAutomaton_ptr any_int = IntAutomaton::makeAnyInt();

  dfaNegation(current_dfa);
  complement_dfa = dfaProduct(any_int->dfa, current_dfa, dfaAND);
  delete any_int;
  any_int = nullptr;
  dfaFree(current_dfa);
  current_dfa = nullptr;

  minimized_dfa = dfaMinimize(complement_dfa);
  dfaFree(complement_dfa);
  complement_dfa = nullptr;

  complement_auto = new IntAutomaton(minimized_dfa, num_of_variables);
  complement_auto->has_negative_1 = (not this->has_negative_1);

  DVLOG(VLOG_LEVEL) << complement_auto->id << " = [" << this->id << "]->makeComplement()";

  return complement_auto;
}

IntAutomaton_ptr IntAutomaton::union_(int value) {
  IntAutomaton_ptr union_auto = nullptr, int_auto = nullptr;
  if (value == -1) {
    union_auto = this->clone();
    union_auto->has_negative_1 = true;
    DVLOG(VLOG_LEVEL) << union_auto->id << " = [" << this->id << "]->union(-1)";
  } else {
    int_auto = IntAutomaton::makeInt(value);
    union_auto = this->union_(int_auto);
    delete int_auto; int_auto = nullptr;
  }
  return union_auto;
}

IntAutomaton_ptr IntAutomaton::union_(IntAutomaton_ptr other_auto) {
  DFA_ptr union_dfa = nullptr, minimized_dfa = nullptr;
  IntAutomaton_ptr union_auto = nullptr;

  union_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaOR);
  minimized_dfa = dfaMinimize(union_dfa);
  dfaFree(union_dfa);
  union_dfa = nullptr;

  union_auto = new IntAutomaton(minimized_dfa, num_of_variables);
  union_auto->has_negative_1 = this->has_negative_1 or other_auto->has_negative_1;

  DVLOG(VLOG_LEVEL) << union_auto->id << " = [" << this->id << "]->union(" << other_auto->id << ")";

  return union_auto;
}

IntAutomaton_ptr IntAutomaton::intersect(int value) {
  IntAutomaton_ptr intersect_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeInt(value);
  intersect_auto = this->intersect(int_auto);
  delete int_auto;
  return intersect_auto;
}

IntAutomaton_ptr IntAutomaton::intersect(IntAutomaton_ptr other_auto) {
  DFA_ptr intersect_dfa = nullptr, minimized_dfa = nullptr;
  IntAutomaton_ptr intersect_auto = nullptr;

  intersect_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaAND);
  minimized_dfa = dfaMinimize(intersect_dfa);
  dfaFree(intersect_dfa);
  intersect_dfa = nullptr;

  intersect_auto = new IntAutomaton(minimized_dfa, num_of_variables);
  intersect_auto->has_negative_1 = this->has_negative_1 and other_auto->has_negative_1;

  DVLOG(VLOG_LEVEL) << intersect_auto->id << " = [" << this->id << "]->intersect(" << other_auto->id << ")";

  return intersect_auto;
}

IntAutomaton_ptr IntAutomaton::difference(int value) {
  IntAutomaton_ptr difference_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeInt(value);
  difference_auto = this->difference(int_auto);
  delete int_auto;
  return difference_auto;
}

IntAutomaton_ptr IntAutomaton::difference(IntAutomaton_ptr other_auto) {
  IntAutomaton_ptr difference_auto = nullptr, complement_auto = nullptr;

  complement_auto = other_auto->complement();
  difference_auto = this->intersect(complement_auto);
  // negative one handled in complement and intersect

  delete complement_auto; complement_auto = nullptr;

  DVLOG(VLOG_LEVEL) << difference_auto->id << " = [" << this->id << "]->difference(" << other_auto->id << ")";

  return difference_auto;
}

IntAutomaton_ptr IntAutomaton::uminus() {
  IntAutomaton_ptr u_minus_auto = nullptr, tmp_auto_2 = nullptr, tmp_auto_1 = nullptr;
  bool has_zero = hasZero();
  bool is_singleton = isAcceptingSingleInt();
  bool is = has_negative_1;

  if (has_negative_1 and has_zero) {
    u_minus_auto = IntAutomaton::makeIntRange(0,1);
  } else if (has_negative_1) {
    u_minus_auto = IntAutomaton::makeInt(1);
  } else if (has_zero) {
    u_minus_auto = IntAutomaton::makeInt(0);
  }

  tmp_auto_1 = IntAutomaton::makeIntGreaterThan(0);
  tmp_auto_2 = this->intersect(tmp_auto_1);
  delete tmp_auto_1;
  if (not tmp_auto_2->isEmptyLanguage()) {
    u_minus_auto->has_negative_1 = true;
  }

  DVLOG(VLOG_LEVEL) << u_minus_auto->id << " = [" << this->id << "]->uminus()";

  return u_minus_auto;
}

IntAutomaton_ptr IntAutomaton::plus(int value) {
  IntAutomaton_ptr plus_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeInt(value);

  plus_auto = this->plus(int_auto);

  delete int_auto; int_auto = nullptr;
  return plus_auto;
}

IntAutomaton_ptr IntAutomaton::plus(IntAutomaton_ptr other_auto) {
  IntAutomaton_ptr plus_auto = nullptr, add_minus_auto = nullptr,
          left_auto = this, right_auto = other_auto;
  if (has_negative_1) {
    add_minus_auto = other_auto->minus(1);
    right_auto = other_auto->union_(add_minus_auto);
    delete add_minus_auto; add_minus_auto = nullptr;
  }

  if (other_auto->has_negative_1) {
    add_minus_auto = this->minus(1);
    left_auto = other_auto->union_(add_minus_auto);
    delete add_minus_auto; add_minus_auto = nullptr;
  }

  plus_auto = left_auto->__plus(right_auto);
  if (other_auto->has_negative_1) {
    delete left_auto; left_auto = nullptr;
  }
  if (has_negative_1) {
    delete right_auto; right_auto = nullptr;
  }

  DVLOG(VLOG_LEVEL) << plus_auto->id << " = [" << this->id << "]->plus(" << other_auto->id << ")";

  return plus_auto;
}

IntAutomaton_ptr IntAutomaton::times(int value) {
  IntAutomaton_ptr times_auto = nullptr, tmp_auto = nullptr;
  if (value == 0) {
    times_auto = IntAutomaton::makeZero();
  } else if (value == 1) {
    times_auto = this->clone();
  } else if (value == -1) {
    times_auto = this->uminus();
  } else {
    int bound = (value > 0) ? value : -value;
    times_auto = this->clone();

    for (int i = 1; i < value; i++) {
      tmp_auto = times_auto;
      times_auto = tmp_auto->plus(this);
      delete tmp_auto; tmp_auto = nullptr;
    }

    if (value < 0) {
      tmp_auto = times_auto;
      times_auto = tmp_auto->uminus();
      delete tmp_auto;
    }
  }

  DVLOG(VLOG_LEVEL) << times_auto->id << " = [" << this->id << "]->times(" << value << ")";

  return times_auto;
}


IntAutomaton_ptr IntAutomaton::minus(int value) {
  IntAutomaton_ptr minus_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeInt(value);
  minus_auto = this->minus(int_auto);
  delete int_auto; int_auto = nullptr;
  return minus_auto;
}

/**
 * TODO compare max and min values to decide if minus one is in final results
 */
IntAutomaton_ptr IntAutomaton::minus(IntAutomaton_ptr other_auto) {
  IntAutomaton_ptr minus_auto = nullptr, add_plus_auto = nullptr,
          left_auto = this, right_auto = other_auto;

  if (other_auto->has_negative_1) {
    add_plus_auto = this->plus(1);
    left_auto = this->union_(add_plus_auto);
    delete add_plus_auto;
  }

  minus_auto = left_auto->__minus(right_auto);
  if (other_auto->has_negative_1) {
    delete left_auto; left_auto = nullptr;
  }

  DVLOG(VLOG_LEVEL) << minus_auto->id << " = [" << this->id << "]->plus(" << other_auto->id << ")";

  return minus_auto;
}

IntAutomaton_ptr IntAutomaton::substractFrom(int value) {
  IntAutomaton_ptr minus_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeInt(value);
  minus_auto = int_auto->minus(this);
  delete int_auto;
  return minus_auto;
}

int IntAutomaton::getMaxAcceptedInt() {
  if (this->isCyclic()) {
    return IntAutomaton::INFINITE;
  } else if (this->isAcceptingSingleInt()) {
    return this->getAnAcceptingInt();
  }

  AdjacencyList adjacency_count_list = this->getAdjacencyCountList();
  adjacency_count_list[this->getSinkState()] = NodeVector(0);

  const int n = adjacency_count_list.size();
  int y;
  int * u = new int[n];
  int * v = new int[n];
  int max_int = 0;
  std::vector<int> final_states;

  for(int j = 0; j < this->dfa->ns; j++) {
    if (isAcceptingState(j)) {
      final_states.push_back(j);
    }
  }

  for(int j = 0; j < n; j++) {
    v[j] = 0; u[j] = 0;
  }

  v[this->dfa->s] = 1;

  for (int col = 0; col < n; col++) {
    int c = 0;
    for (int j = 0; j < n; j ++) {
      u[j] = 0;
    }
    for (int ii = 0; ii < n; ii++) {
      for(int jj = 0; jj < (int)adjacency_count_list[ii].size(); jj++) {
          y = adjacency_count_list[ii][jj].first;
          u[c] |= v[y];
      }
      c++;
    }
    for (int d = 0; d < (int)final_states.size(); d++) {
      if (v[final_states[d]]) {
        max_int = col;
      }
    }
    for (int b = 0; b < n; b++) {
      v[b] = u[b];
    }
  }
  delete[] u;
  delete[] v;
  return max_int;
}

int IntAutomaton::getMinAcceptedInt() {
  if (has_negative_1) {
    return -1;
  } else if (this->isAcceptingSingleInt()) {
    return this->getAnAcceptingInt();
  }

  AdjacencyList adjacency_count_list = this->getAdjacencyCountList();
  adjacency_count_list[this->getSinkState()] = NodeVector(0);

  const int n = adjacency_count_list.size();
  int j, col;
  int y;
  int * u = new int[n];
  int * v = new int[n];
  int min_int = 0;
  bool min_int_found = false;
  std::vector<int> final_states;

  for(int j = 0; j < this->dfa->ns; j++) {
    if (isAcceptingState(j)) {
      final_states.push_back(j);
    }
  }

  for (int j = 0; j < n; j++) {
    v[j] = 0; u[j] = 0;
  }

  v[this->dfa->s] = 1;

  for (int col = 0; (col < n) and (not min_int_found); col++) {
    int c = 0;
    for(int j = 0; j < n; j ++) {
      u[j] = 0;
    }
    for (int ii = 0; ii < n; ii++){
      for(int jj = 0; jj < (int)adjacency_count_list[ii].size(); jj++){
          y = adjacency_count_list[ii][jj].first;
          u[c] |= v[y];
      }
      c++;
    }
    for (int d = 0; (not min_int_found) and (d < (int)final_states.size()); d++) {
      if (v[final_states[d]]) {
        min_int = col;
        min_int_found = true;
      }
    }
    for (int b = 0; b < n; b++) {
      v[b] = u[b];
    }
  }
  delete[] u;
  delete[] v;
  return min_int;
}

bool IntAutomaton::isGreaterThan(int value) {
  if (this->isEmptyLanguage()) {
    return false;
  }
  int max_int = this->getMaxAcceptedInt();
  if (max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    return (max_int > value);
  }
}

bool IntAutomaton::isGreaterThan(IntAutomaton_ptr other_auto) {
  if (this->isEmptyLanguage() or other_auto->isEmptyLanguage()) {
    return false;
  }
  int left_max_int = this->getMaxAcceptedInt();
  if (left_max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    int right_min_int = other_auto->getMinAcceptedInt();
    return (left_max_int > right_min_int);
  }
}

bool IntAutomaton::isGreaterThanOrEqual(int value) {
  if (this->isEmptyLanguage()) {
    return false;
  }
  int max_int = this->getMaxAcceptedInt();
  if (max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    return (max_int >= value);
  }
}

bool IntAutomaton::isGreaterThanOrEqual(IntAutomaton_ptr other_auto) {
  if (this->isEmptyLanguage() or other_auto->isEmptyLanguage()) {
    return false;
  }
  int left_max_int = this->getMaxAcceptedInt();
  if (left_max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    int right_min_int = other_auto->getMinAcceptedInt();
    return (left_max_int >= right_min_int);
  }
}

bool IntAutomaton::isLessThan(int value) {
  if (this->isEmptyLanguage()) {
    return false;
  }
  return (this->getMinAcceptedInt() < value);
}

bool IntAutomaton::isLessThan(IntAutomaton_ptr other_auto) {
  if (this->isEmptyLanguage() or other_auto->isEmptyLanguage()) {
    return false;
  }
  int right_max_int = other_auto->getMaxAcceptedInt();
  if (right_max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    int left_min_int = this->getMinAcceptedInt();
    return (left_min_int < right_max_int);
  }
}

bool IntAutomaton::isLessThanOrEqual(int value) {
  if (this->isEmptyLanguage()) {
    return false;
  }
  return (this->getMinAcceptedInt() <= value);
}

bool IntAutomaton::isLessThanOrEqual(IntAutomaton_ptr other_auto) {
  if (this->isEmptyLanguage() or other_auto->isEmptyLanguage()) {
    return false;
  }
  int right_max_int = other_auto->getMaxAcceptedInt();
  if (right_max_int == IntAutomaton::INFINITE) {
    return true;
  } else {
    int left_min_int = this->getMinAcceptedInt();
    return (left_min_int <= right_max_int);
  }
}

IntAutomaton_ptr IntAutomaton::restrictTo(IntAutomaton_ptr other_value) {
  IntAutomaton_ptr restricted_auto = nullptr;
  restricted_auto = this->intersect(other_value);

  DVLOG(VLOG_LEVEL) << this->id << " = [" << this->id << "]->restrict(" << other_value->id << ")";
  return restricted_auto;
}

IntAutomaton_ptr IntAutomaton::restrictGreaterThanTo(int value) {
  IntAutomaton_ptr restricted_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeIntGreaterThan(value);
  restricted_auto = this->restrictTo(int_auto);
  delete int_auto;
  return restricted_auto;
}

IntAutomaton_ptr IntAutomaton::restrictGreaterThanTo(IntAutomaton_ptr other_auto) {
  if (other_auto->isEmptyLanguage()) {
    return IntAutomaton::makePhi();
  }

  int min_int = other_auto->getMinAcceptedInt();
  return this->restrictGreaterThanTo(min_int);
}

IntAutomaton_ptr IntAutomaton::restrictGreaterThanOrEqualTo(int value) {
  IntAutomaton_ptr restricted_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeIntGreaterThanOrEqual(value);
  restricted_auto = this->restrictTo(int_auto);
  delete int_auto;
  return restricted_auto;
}

IntAutomaton_ptr IntAutomaton::restrictGreaterThanOrEqualTo(IntAutomaton_ptr other_auto) {
  if (other_auto->isEmptyLanguage()) {
    return IntAutomaton::makePhi();
  }

  int min_int = other_auto->getMinAcceptedInt();
  return this->restrictGreaterThanOrEqualTo(min_int);
}

IntAutomaton_ptr IntAutomaton::restrictLessThanTo(int value) {
  IntAutomaton_ptr restricted_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeIntLessThan(value);
  restricted_auto = this->restrictTo(int_auto);
  delete int_auto;
  return restricted_auto;
}

IntAutomaton_ptr IntAutomaton::restrictLessThanTo(IntAutomaton_ptr other_auto) {
  if (other_auto->isEmptyLanguage()) {
    return IntAutomaton::makePhi();
  }

  int max_int = other_auto->getMaxAcceptedInt();
  if (max_int != IntAutomaton::INFINITE) {
    return this->restrictLessThanTo(max_int);
  } else {
    return this->clone();
  }
}

IntAutomaton_ptr IntAutomaton::restrictLessThanOrEqualTo(int value) {
  IntAutomaton_ptr restricted_auto = nullptr, int_auto = nullptr;
  int_auto = IntAutomaton::makeIntLessThanOrEqual(value);
  restricted_auto = this->restrictTo(int_auto);
  delete int_auto;
  return restricted_auto;
}

IntAutomaton_ptr IntAutomaton::restrictLessThanOrEqualTo(IntAutomaton_ptr other_auto) {
  if (other_auto->isEmptyLanguage()) {
    return IntAutomaton::makePhi();
  }

  int max_int = other_auto->getMaxAcceptedInt();
  if (max_int != IntAutomaton::INFINITE) {
    return this->restrictLessThanOrEqualTo(max_int);
  } else {
    return this->clone();
  }
}

bool IntAutomaton::checkEquivalance(IntAutomaton_ptr other_auto) {
  return (Automaton::IsEqual(other_auto) and (has_negative_1 == other_auto->has_negative_1));
}

bool IntAutomaton::isEmptyLanguage() {
  if (has_negative_1) {
    return false;
  }
  return Automaton::isEmptyLanguage();
}

bool IntAutomaton::hasZero() {
  return Automaton::isInitialStateAccepting();
}

bool IntAutomaton::isZero() {
  return (Automaton::isOnlyInitialStateAccepting() and (not has_negative_1));
}

bool IntAutomaton::isAcceptingSingleInt() {
  int sink_state = getSinkState(),
      curr_state = -1,
      num_of_accepting_paths = 0;
  std::stack<int> state_path;
  std::set<int> next_states;

  state_path.push(this->dfa->s);
  while (not state_path.empty()) {
    curr_state = state_path.top(); state_path.pop();
    if (this->isAcceptingState(curr_state)) {
      ++num_of_accepting_paths;
    }
    if (num_of_accepting_paths > 1) {
      return false;
    }
    next_states = getNextStates(curr_state);
    next_states.erase(sink_state);
    for (int next_state : next_states) {
      state_path.push(next_state);
    }
  }

  return ((num_of_accepting_paths == 1) not_eq has_negative_1);
}

/**
 * TODO update getAnAcceptingWord to generate string in all case except bottom
 */
int IntAutomaton::getAnAcceptingInt() {
  int sink_state = getSinkState(),
      curr_state = -1,
      num_of_accepting_paths = 0;
  std::stack<int> state_path;
  std::stack<int> path_length_stack;
  int path_length = 0;
  std::set<int> next_states;

  state_path.push(this->dfa->s);
  path_length_stack.push(0);
  while (not state_path.empty()) {
    curr_state = state_path.top(); state_path.pop();
    path_length = path_length_stack.top(); path_length_stack.pop();
    if (this->isAcceptingState(curr_state)) {
      return path_length;
    }
    next_states = this->getNextStates(curr_state);
    next_states.erase(sink_state);
    for (int next_state : next_states) {
      state_path.push(next_state);
      path_length_stack.push(path_length + 1);
    }
  }

  if (has_negative_1) {
    return -1;
  }

  return -2; // not accepting
}

UnaryAutomaton_ptr IntAutomaton::toUnaryAutomaton() {
  UnaryAutomaton_ptr unary_auto = nullptr;
  DFA_ptr unary_dfa = nullptr;
  int number_of_variables = 1;
  int* indices = getIndices(number_of_variables);
  const int number_of_states = this->dfa->ns;
  int to_state, sink_state = getSinkState();
  std::vector<char> unary_exception = {'1'};
  char* statuses = new char[number_of_states + 1];
  std::vector<char> exception = {'0', '0', '0', '0', '0', '0', '0', '0'};

  dfaSetup(number_of_states, number_of_variables, indices);

  for (int s = 0; s < this->dfa->ns; s++) {
    to_state = getNextState(s, exception);
    dfaAllocExceptions(1);
    dfaStoreException(to_state, &*unary_exception.begin());

    dfaStoreState(sink_state);

    if (isAcceptingState(s)) {
      statuses[s] = '+';
    } else {
      statuses[s] = '-';
    }
  }
  statuses[number_of_states] = '\0';
  dfaAllocExceptions(0);
  dfaStoreState(sink_state);

  unary_dfa = dfaBuild(statuses);

  delete[] indices; indices = nullptr;
  delete[] statuses;

  unary_auto = new UnaryAutomaton(unary_dfa);
  DVLOG(VLOG_LEVEL) << unary_auto->getId() << " = [" << this->id << "]->toUnaryAutomaton()";
  return unary_auto;
}

/**
 * Should be same as string concat
 */
IntAutomaton_ptr IntAutomaton::__plus(IntAutomaton_ptr other_auto) {
  DFA_ptr concat_dfa = nullptr, tmp_dfa = nullptr;
  IntAutomaton_ptr concat_auto = nullptr, to_union_auto = nullptr;

  if (this->isEmptyLanguage() or other_auto->isEmptyLanguage()) {
    return IntAutomaton::makePhi();
  } else if (this->isZero()) {
    return other_auto->clone();
  } else if (other_auto->isZero()) {
    return this->clone();
  }

  bool has_empty_string = other_auto->hasZero();
  bool delete_other_auto = false;


  if (has_empty_string and other_auto->hasIncomingTransition(other_auto->dfa->s)) {
    LOG(FATAL) << "implement me";
//    DFA_ptr shifted_dfa = dfa_shift_empty_M(other_auto->dfa, other_auto->num_of_variables, other_auto->variable_indices);
//    IntAutomaton_ptr shifted_auto = new IntAutomaton(shifted_dfa, other_auto->num_of_variables);
//    other_auto = shifted_auto;
//    delete_other_auto = true;
  }

  int var = num_of_variables;
  int* indices = variable_indices;
  int tmp_num_of_variables,
      state_id_shift_amount,
      expected_num_of_states,
      sink_state_left_auto,
      sink_state_right_auto,
      to_state = 0,
      loc,
      i = 0,
      j = 0;

  bool is_start_state_reachable;
  paths state_paths = nullptr, pp = nullptr;
  trace_descr tp = nullptr;

  std::map<std::vector<char>*, int> exceptions_left_auto;
  std::map<std::vector<char>*, int> exceptions_right_auto;
  std::map<std::vector<char>*, int> exceptions_fix;
  std::vector<char>* current_exception = nullptr;
  char* statuses = nullptr;

  // variable initializations
  sink_state_left_auto = this->getSinkState();
  sink_state_right_auto = other_auto->getSinkState();

  CHECK_GT(sink_state_left_auto, -1);
  CHECK_GT(sink_state_right_auto, -1);

  tmp_num_of_variables = this->num_of_variables + 1; // add one extra bit
  state_id_shift_amount = this->dfa->ns;

  expected_num_of_states = this->dfa->ns + other_auto->dfa->ns - 1; // -1 is for to remove one of the sink states
  is_start_state_reachable = other_auto->isStartStateReachableFromAnAcceptingState();
  if (not is_start_state_reachable) {
    expected_num_of_states = expected_num_of_states  - 1; // if start state is reachable from an accepting state, it will be merge with accepting states of left hand side
  }

  statuses = new char[expected_num_of_states + 1];
  int* concat_indices = getIndices(tmp_num_of_variables);

  dfaSetup(expected_num_of_states, tmp_num_of_variables, concat_indices); //sink states are merged
  state_paths = pp = make_paths(other_auto->dfa->bddm, other_auto->dfa->q[other_auto->dfa->s]);
  while (pp) {
    if ( pp->to != (unsigned)sink_state_right_auto ) {
      to_state = pp->to + state_id_shift_amount;
      // if there is a self loop keep it
      if ( pp->to == (unsigned)other_auto->dfa->s ) {
        to_state -= 2;
      } else {
        if ( sink_state_right_auto >= 0 && pp->to > (unsigned)sink_state_right_auto ) {
          to_state--; //to new state, sink state will be eliminated and hence need -1
        }
        if ((not is_start_state_reachable) && pp->to > (unsigned)other_auto->dfa->s) {
          to_state--; // to new state, init state will be eliminated if init is not reachable
        }
      }

      current_exception = new std::vector<char>();
      for (j = 0; j < other_auto->num_of_variables; j++) {
        //the following for loop can be avoided if the indices are in order
        for (tp = pp->trace; tp && (tp->index != (unsigned)indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) {
            current_exception->push_back('1');
          }
          else {
            current_exception->push_back('0');
          }
        }
        else {
          current_exception->push_back('X');
        }
      }

      current_exception->push_back('1'); // new path
      current_exception->push_back('\0');
      exceptions_right_auto[current_exception] = to_state;
    }
    current_exception = nullptr;
    tp = nullptr;
    pp = pp->next;
  }

  kill_paths(state_paths);
  state_paths = pp = nullptr;

  for (i = 0; i < this->dfa->ns; i++) {
    state_paths = pp = make_paths(this->dfa->bddm, this->dfa->q[i]);
    while (pp) {
      if (pp->to == (unsigned)sink_state_left_auto) {
        pp = pp->next;
        continue;
      }
      to_state = pp->to;
      current_exception = new std::vector<char>();
      for (j = 0; j < this->num_of_variables; j++) {
        for (tp = pp->trace; tp && (tp->index != (unsigned)indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) {
            current_exception->push_back('1');
          } else {
            current_exception->push_back('0');
          }
        } else {
          current_exception->push_back('X');
        }
      }

      current_exception->push_back('0'); // add extra bit, '0' is used for the exceptions coming from left auto
      current_exception->push_back('\0');
      exceptions_left_auto[current_exception] = to_state;
      tp = nullptr;
      pp = pp->next;
    }
    current_exception = nullptr;
    // generate concat automaton
    if (this->isAcceptingState(i)) {
      dfaAllocExceptions(exceptions_left_auto.size() + exceptions_right_auto.size());
      for (auto it = exceptions_left_auto.begin(); it != exceptions_left_auto.end();) {
        dfaStoreException(it->second, &*it->first->begin());
        current_exception = it->first;
        it = exceptions_left_auto.erase(it);
        delete current_exception;
      }
      for (auto it = exceptions_right_auto.begin(); it != exceptions_right_auto.end();) {
        dfaStoreException(it->second, &*it->first->begin());
        current_exception = it->first;
        it = exceptions_right_auto.erase(it);
        delete current_exception;
      }

      dfaStoreState(sink_state_left_auto);
      if (other_auto->isAcceptingState(0)) {
        statuses[i]='+';
      }
      else {
        statuses[i]='-';
      }
    } else {
      dfaAllocExceptions(exceptions_left_auto.size());
      for (auto it = exceptions_left_auto.begin(); it != exceptions_left_auto.end();) {
        dfaStoreException(it->second, &*it->first->begin());
        current_exception = it->first;
        it = exceptions_left_auto.erase(it);
        delete current_exception;
      }
      dfaStoreState(sink_state_left_auto);
      statuses[i] = '-';
    }
    current_exception = nullptr;
    kill_paths(state_paths);
    state_paths = pp = nullptr;
  }

  //  initflag is 1 iff init is reached by some state. In this case,
  for (i = 0; i < other_auto->dfa->ns; i++) {
    if ( i != sink_state_right_auto ) {
      if ( i != other_auto->dfa->s || is_start_state_reachable) {
        state_paths = pp = make_paths(other_auto->dfa->bddm, other_auto->dfa->q[i]);
        while (pp) {
          if ( pp->to != (unsigned)sink_state_right_auto) {
            to_state = pp->to + state_id_shift_amount;

            if ( sink_state_right_auto >= 0 && pp->to > (unsigned)sink_state_right_auto) {
              to_state--; //to new state, sink state will be eliminated and hence need -1
            }

            if ( (not is_start_state_reachable) && pp->to > (unsigned)other_auto->dfa->s) {
              to_state--; // to new state, init state will be eliminated if init is not reachable
            }

            current_exception = new std::vector<char>();
            for (j = 0; j < var; j++) {
              for (tp = pp->trace; tp && (tp->index != (unsigned)indices[j]); tp =tp->next);
              if (tp) {
                if (tp->value){
                  current_exception->push_back('1');
                }
                else {
                  current_exception->push_back('0');
                }
              }
              else {
                current_exception->push_back('X');
              }
            }
            current_exception->push_back('0'); // old value
            current_exception->push_back('\0');
            exceptions_fix[current_exception] = to_state;
            tp = nullptr;
            current_exception = nullptr;
          }
          pp = pp->next;
        }

        dfaAllocExceptions(exceptions_fix.size());
        for (auto it = exceptions_fix.begin(); it != exceptions_fix.end();) {
          dfaStoreException(it->second, &*it->first->begin());
          current_exception = it->first;
          it = exceptions_fix.erase(it);
          delete current_exception;
        }

        dfaStoreState(sink_state_left_auto);

        loc = state_id_shift_amount + i;
        if ( (not is_start_state_reachable) && i > other_auto->dfa->s) {
          loc--;
        }
        if ( sink_state_right_auto >= 0 && i > sink_state_right_auto) {
          loc--;
        }

        if ( other_auto->isAcceptingState(i)) {
          statuses[loc]='+';
        } else {
          statuses[loc]='-';
        }

        kill_paths(state_paths);
        state_paths = pp = nullptr;
      }
    }
  }

  statuses[expected_num_of_states]='\0';

  concat_dfa = dfaBuild(statuses);
  delete[] statuses; statuses = nullptr;
  delete[] concat_indices; concat_indices = nullptr;
  tmp_dfa = dfaProject(concat_dfa, (unsigned) var);
  dfaFree(concat_dfa);
  concat_dfa = dfaMinimize(tmp_dfa);
  dfaFree(tmp_dfa); tmp_dfa = nullptr;

  concat_auto = new IntAutomaton(concat_dfa, num_of_variables);

  if (has_empty_string) {
    IntAutomaton_ptr tmp_auto = concat_auto;
    concat_auto = tmp_auto->union_(this);
    delete tmp_auto;
    if (delete_other_auto) {
      delete other_auto;
    }
  }

  DVLOG(VLOG_LEVEL) << concat_auto->id << " = [" << this->id << "]->__plus(" << other_auto->id << ")";

  return concat_auto;
}

/**
 * Fix dfa preconcat bug
 */
IntAutomaton_ptr IntAutomaton::__minus(IntAutomaton_ptr other_auto) {
  DFA_ptr result_dfa = nullptr;
  IntAutomaton_ptr result_auto = nullptr;

  LOG(FATAL) << "implement me";
//  result_dfa = dfa_pre_concat(this->dfa, other_auto->dfa, 1, num_of_variables, variable_indices);

  result_auto = new IntAutomaton(result_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->__minus(" << other_auto->id << ")";

  return result_auto;
}


} /* namespace Theory */
} /* namespace Vlab */
