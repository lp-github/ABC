/*
 * StringAutomaton.cpp
 *
 *  Created on: Jun 24, 2015
 *      Author: baki
 */

#include "StringAutomaton.h"

namespace Vlab {
namespace Theory {

const int StringAutomaton::VLOG_LEVEL = 8;

int StringAutomaton::DEFAULT_NUM_OF_VARIABLES = 8;

int* StringAutomaton::DEFAULT_VARIABLE_INDICES = StringAutomaton::allocateAscIIIndexWithExtraBit(
        StringAutomaton::DEFAULT_NUM_OF_VARIABLES);

unsigned* StringAutomaton::DEFAULT_UNSIGNED_VARIABLE_INDICES = StringAutomaton::get_unsigned_indices_main(
        StringAutomaton::DEFAULT_NUM_OF_VARIABLES);

StringAutomaton::StringAutomaton(DFA_ptr dfa)
        : Automaton(Automaton::Type::STRING), dfa (dfa), num_of_variables(StringAutomaton::DEFAULT_NUM_OF_VARIABLES) {
}

StringAutomaton::StringAutomaton(DFA_ptr dfa, int num_of_variables)
        : Automaton(Automaton::Type::STRING), dfa(dfa), num_of_variables(num_of_variables) {
}

StringAutomaton::StringAutomaton(const StringAutomaton& other)
        : Automaton(Automaton::Type::STRING), dfa(dfaCopy(other.dfa)), num_of_variables(other.num_of_variables) {
}

StringAutomaton::~StringAutomaton() {
//  DVLOG(VLOG_LEVEL) << "delete " << " [" << this->id << "]";
  dfaFree(dfa);
  dfa = nullptr;
}

StringAutomaton_ptr StringAutomaton::clone() const {
  StringAutomaton_ptr cloned_auto = new StringAutomaton(*this);
  DVLOG(VLOG_LEVEL) << cloned_auto->id << " = [" << this->id << "]->clone()";
  return cloned_auto;
}

/**
 * Creates an automaton that accepts nothing
 */
StringAutomaton_ptr StringAutomaton::makePhi(int num_of_variables, int* variable_indices) {
  DFA_ptr non_value_string_dfa = nullptr;
  StringAutomaton_ptr non_value_string = nullptr;
  std::array<char, 1> statuses { '-' };
  dfaSetup(1, num_of_variables, variable_indices);
  dfaAllocExceptions(0);
  dfaStoreState(0);
  non_value_string_dfa = dfaBuild(&*statuses.begin());
  non_value_string = new StringAutomaton(non_value_string_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << non_value_string->id << " = makePhi()";

  return non_value_string;
}

StringAutomaton_ptr StringAutomaton::makeEmptyString(int num_of_variables, int* variable_indices) {
  DFA_ptr empty_string_dfa = nullptr;
  StringAutomaton_ptr empty_string = nullptr;
  std::array<char, 2> statuses { '+', '-' };

  dfaSetup(2, num_of_variables, variable_indices);
  dfaAllocExceptions(0);
  dfaStoreState(1);
  dfaAllocExceptions(0);
  dfaStoreState(1);
  empty_string_dfa = dfaBuild(&*statuses.begin());
  empty_string = new StringAutomaton(empty_string_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << empty_string->id << " = makeEmptyString()";

  return empty_string;
}

StringAutomaton_ptr StringAutomaton::makeString(std::string str, int num_of_variables, int* variable_indices) {
  if (str.empty()) {
    return StringAutomaton::makeEmptyString();
  }

  char* binary_char = nullptr;
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;
  int str_length = str.length();
  int number_of_states = str_length + 2;
  char* statuses = new char[number_of_states];

  dfaSetup(number_of_states, num_of_variables, variable_indices);

  int i;
  for (i = 0; i < str_length; i++) {
    dfaAllocExceptions(1);
    binary_char = StringAutomaton::binaryFormat((unsigned long) str[i], num_of_variables);
    dfaStoreException(i + 1, binary_char);
    delete binary_char;
    dfaStoreState(str_length + 1);
    statuses[i] = '-';
  }

  dfaAllocExceptions(0);
  dfaStoreState(str_length + 1);
  statuses[str_length] = '+';
  CHECK_EQ(str_length, i);

  //sink state
  dfaAllocExceptions(0);
  dfaStoreState(str_length + 1);
  statuses[str_length + 1] = '-';

  result_dfa = dfaBuild(statuses);
  result_auto = new StringAutomaton(result_dfa, num_of_variables);
  delete statuses;

  DVLOG(VLOG_LEVEL) << result_auto->id << " = makeString(\"" << str << "\")";

  return result_auto;
}

/**
 * Returns Sigma* except two reserved words (11111111, 11111110)
 *
 */
StringAutomaton_ptr StringAutomaton::makeAnyString(int num_of_variables, int* variable_indices) {
  DFA_ptr any_string_dfa = nullptr;
  StringAutomaton_ptr any_string = nullptr;
  std::array<char, 2> statuses { '+', '-' };
  std::vector<char> reserved_1 = StringAutomaton::getReservedWord('1', num_of_variables);
  std::vector<char> reserved_2 = StringAutomaton::getReservedWord('0', num_of_variables);
  char* sharp1 = &*reserved_1.begin();
  char* sharp0 = &*reserved_2.begin();

  dfaSetup(2, num_of_variables, variable_indices);
  dfaAllocExceptions(2);
  dfaStoreException(1, sharp1); // word 11111111
  dfaStoreException(1, sharp0); // word 11111110
  dfaStoreState(0);

  dfaAllocExceptions(0);
  dfaStoreState(1);

  any_string_dfa = dfaBuild(&*statuses.begin());
  any_string = new StringAutomaton(any_string_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << any_string->id << " = makeAnyString()";

  return any_string;
}

StringAutomaton_ptr StringAutomaton::makeChar(char c, int num_of_variables, int* variable_indices) {
  std::stringstream ss;
  ss << c;
  return StringAutomaton::makeString(ss.str(), num_of_variables, variable_indices);
}

/**
 * Generates dfa for range [from-to]
 */
StringAutomaton_ptr StringAutomaton::makeCharRange(char from, char to, int num_of_variables, int* variable_indices) {
  int from_char = (int) from;
  int to_char = (int) to;
  int index;
  int initial_state;
  std::array<char, 3> statuses { '-', '+', '-' };
  DFA_ptr range_dfa = nullptr;
  StringAutomaton_ptr range_auto = nullptr;

  if (from_char > to_char) {
    int tmp = from_char;
    from_char = to_char;
    to_char = tmp;
  }

  initial_state = to_char - from_char;

  dfaSetup(3, num_of_variables, variable_indices);

  //state 0
  dfaAllocExceptions(initial_state + 1);
  for (index = from_char; index <= to_char; index++) {
    dfaStoreException(1, bintostr(index, num_of_variables));
  }
  dfaStoreState(2);

  //state 1
  dfaAllocExceptions(0);
  dfaStoreState(2);

  //state 2
  dfaAllocExceptions(0);
  dfaStoreState(2);

  range_dfa = dfaBuild(&*statuses.begin());
  range_auto = new StringAutomaton(range_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << range_auto->id << " = makeCharRange('" << from << "', '" << to << "')";

  return range_auto;
}

/**
 * Regular expressions '.' operator
 */
StringAutomaton_ptr StringAutomaton::makeAnyChar(int num_of_variables, int* variable_indices) {
  std::array<char, 3> statuses { '-', '+', '-' };
  std::vector<char> reserved_1 = StringAutomaton::getReservedWord('1', num_of_variables);
  std::vector<char> reserved_2 = StringAutomaton::getReservedWord('0', num_of_variables);
  char* sharp1 = &*reserved_1.begin();
  char* sharp0 = &*reserved_2.begin();
  DFA_ptr dot_dfa = nullptr;
  StringAutomaton_ptr dot_auto = nullptr;

  dfaSetup(3, num_of_variables, variable_indices);

  dfaAllocExceptions(2);
  dfaStoreException(2, sharp1); // word 11111111
  dfaStoreException(2, sharp0); // word 11111110
  dfaStoreState(1);
  dfaAllocExceptions(0);
  dfaStoreState(2);
  dfaAllocExceptions(0);
  dfaStoreState(2);

  dot_dfa = dfaBuild(&*statuses.begin());
  dot_auto = new StringAutomaton(dot_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << dot_auto->id << " = makeAnyChar()";

  return dot_auto;
}

StringAutomaton_ptr StringAutomaton::makeRegexAuto(std::string regex, int num_of_variables, int* variable_indices) {
  StringAutomaton_ptr regex_auto = nullptr;

  Util::RegularExpression_ptr regular_expression = new Util::RegularExpression(regex);
  regex_auto = StringAutomaton::makeRegexAuto(regular_expression);

  DVLOG(VLOG_LEVEL) << regex_auto->id << " = makeRegexAuto(" << regex << ")";

  return regex_auto;
}

StringAutomaton_ptr StringAutomaton::makeRegexAuto(Util::RegularExpression_ptr regular_expression) {
  StringAutomaton_ptr regex_auto = nullptr;
  StringAutomaton_ptr regex_expr1_auto = nullptr;
  StringAutomaton_ptr regex_expr2_auto = nullptr;

  switch (regular_expression->getType()) {
  case Util::RegularExpression::Type::UNION:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_expr2_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr2());
    regex_auto = regex_expr1_auto->union_(regex_expr2_auto);
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    delete regex_expr2_auto;
    regex_expr2_auto = nullptr;
    break;
  case Util::RegularExpression::Type::CONCATENATION:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_expr2_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr2());
    regex_auto = regex_expr1_auto->concatenate(regex_expr2_auto);
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    delete regex_expr2_auto;
    regex_expr2_auto = nullptr;
    break;
  case Util::RegularExpression::Type::INTERSECTION:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_expr2_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr2());
    regex_auto = regex_expr1_auto->intersect(regex_expr2_auto);
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    delete regex_expr2_auto;
    regex_expr2_auto = nullptr;
    break;
  case Util::RegularExpression::Type::OPTIONAL:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->optional();
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::REPEAT_STAR:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->kleeneClosure();
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::REPEAT_PLUS:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->closure();
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::REPEAT_MIN:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->repeat(regular_expression->getMin());
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::REPEAT_MINMAX:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->repeat(regular_expression->getMin(), regular_expression->getMax());
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::COMPLEMENT:
    regex_expr1_auto = StringAutomaton::makeRegexAuto(regular_expression->getExpr1());
    regex_auto = regex_expr1_auto->complement();
    delete regex_expr1_auto;
    regex_expr1_auto = nullptr;
    break;
  case Util::RegularExpression::Type::CHAR:
    regex_auto = StringAutomaton::makeChar(regular_expression->getChar());
    break;
  case Util::RegularExpression::Type::CHAR_RANGE:
    regex_auto = StringAutomaton::makeCharRange(regular_expression->getFrom(), regular_expression->getTo());
    break;
  case Util::RegularExpression::Type::ANYCHAR:
    regex_auto = StringAutomaton::makeAnyChar();
    break;
  case Util::RegularExpression::Type::EMPTY:
    regex_auto = StringAutomaton::makePhi();
    break;
  case Util::RegularExpression::Type::STRING:
    regex_auto = StringAutomaton::makeString(regular_expression->getS());
    break;
  case Util::RegularExpression::Type::ANYSTRING:
    regex_auto = StringAutomaton::makeAnyString();
    break;
  case Util::RegularExpression::Type::AUTOMATON:
    LOG(FATAL)<< "Unsupported regular expression" << *regular_expression;
    break;
    case Util::RegularExpression::Type::INTERVAL:
    {
      LOG(FATAL) << "Unsupported regular expression" << *regular_expression;
      break;
    }
    default:
    LOG(FATAL) << "Unsupported regular expression" << *regular_expression;
    break;
  }

  return regex_auto;
}

StringAutomaton_ptr StringAutomaton::makeLengthEqual(int length, int num_of_variables, int* variable_indices){
  StringAutomaton_ptr length_auto = nullptr;
  StringAutomaton_ptr anyChar_auto = nullptr;

  anyChar_auto = StringAutomaton::makeAnyChar();

  if(length < 0){
    length_auto = StringAutomaton::makeAnyString();
  }
  else if (length == 0){
    length_auto = StringAutomaton::makeEmptyString();
  }
  else{
//    length_auto = anyChar_auto->repeat(length,length);

    DFA_ptr length_dfa = dfaStringAutomatonL1toL2(length, length,
             StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
         length_auto = new StringAutomaton(length_dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES);
  }

  delete anyChar_auto;
  anyChar_auto = nullptr;

  DVLOG(VLOG_LEVEL) << length_auto->id << " = makeLength(" << length <<  ")";

  return length_auto;
}

StringAutomaton_ptr StringAutomaton::makeLengthLessThan(int length, int num_of_variables, int* variable_indices){
   StringAutomaton_ptr length_auto = nullptr;
   StringAutomaton_ptr anyChar_auto = nullptr;

   anyChar_auto = StringAutomaton::makeAnyChar();

   if(length < 0){
     length_auto = StringAutomaton::makeAnyString();
   }
   else if (length == 0){
     length_auto = StringAutomaton::makePhi();
   }
   else{
//     length_auto = anyChar_auto->repeat(0,length-1);
     DFA_ptr length_dfa = dfaStringAutomatonL1toL2(0, length-1,
         StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
     length_auto = new StringAutomaton(length_dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES);
   }

   delete anyChar_auto;
   anyChar_auto = nullptr;

   DVLOG(VLOG_LEVEL) << length_auto->id << " = makeLengthLessThan(" << length <<  ")";

   return length_auto;
}

StringAutomaton_ptr StringAutomaton::makeLengthLessThanEqual(int length, int num_of_variables, int* variable_indices){
  StringAutomaton_ptr length_auto = nullptr;
  StringAutomaton_ptr anyChar_auto = nullptr;

  anyChar_auto = StringAutomaton::makeAnyChar();

  if(length < 0){
    length_auto = StringAutomaton::makeAnyString();
  }
  else if (length == 0){
    length_auto = StringAutomaton::makeEmptyString();
  }
  else{
//    length_auto = anyChar_auto->repeat(0,length);

    DFA_ptr length_dfa = dfaStringAutomatonL1toL2(0, length,
             StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
         length_auto = new StringAutomaton(length_dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES);
  }

  delete anyChar_auto;
  anyChar_auto = nullptr;

  DVLOG(VLOG_LEVEL) << length_auto->id << " = makeLengthLessThanEqual(" << length <<  ")";

  return length_auto;
}

StringAutomaton_ptr StringAutomaton::makeLengthGreaterThan(int length, int num_of_variables, int* variable_indices){
  StringAutomaton_ptr length_auto = nullptr;

  if(length < 0){
    length_auto = StringAutomaton::makeAnyString();
  }
  else{
    length_auto = StringAutomaton::makeLengthLessThanEqual(length)->complement();
  }

  DVLOG(VLOG_LEVEL) << length_auto->id << " = makeLengthGreaterThan(" << length <<  ")";

  return length_auto;
}

StringAutomaton_ptr StringAutomaton::makeLengthGreaterThanEqual(int length, int num_of_variables, int* variable_indices){
  StringAutomaton_ptr length_auto = nullptr;

  if(length < 0){
    length_auto = StringAutomaton::makeAnyString();
  }
  else{
    length_auto = StringAutomaton::makeLengthLessThan(length)->complement();
  }

  DVLOG(VLOG_LEVEL) << length_auto->id << " = makeLengthGreaterThanEqual(" << length <<  ")";

  return length_auto;
}


StringAutomaton_ptr StringAutomaton::makeLengthRange(int start, int end, int num_of_variables, int* variable_indices){
  StringAutomaton_ptr range_auto = nullptr, lessThan_auto = nullptr, greaterThanEqual_auto = nullptr;

  greaterThanEqual_auto = StringAutomaton::makeLengthGreaterThanEqual(start);
  lessThan_auto = StringAutomaton::makeLengthLessThan(end);
  range_auto = lessThan_auto->intersect(greaterThanEqual_auto);

  delete greaterThanEqual_auto;
  delete lessThan_auto;

  DVLOG(VLOG_LEVEL) << range_auto->id << " = makeLengthRange(" << start << "," << end <<  ")";

  return range_auto;
}

StringAutomaton_ptr StringAutomaton::minimize(){
  DFA_ptr minimized_dfa = nullptr;
  StringAutomaton_ptr minimized_auto = nullptr;

  minimized_dfa = dfaMinimize(dfaCopy(dfa));
  minimized_auto = new StringAutomaton(minimized_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << minimized_auto->id << " = [" << this->id << "]->minimize()";

  return minimized_auto;
}

/**
 * TODO Try to avoid intersection during complement, figure out a better way
 *
 */
StringAutomaton_ptr StringAutomaton::complement() {
  DFA_ptr complement_dfa = nullptr, minimized_dfa = nullptr, current_dfa = dfaCopy(dfa);
  StringAutomaton_ptr complement_auto = nullptr;
  StringAutomaton_ptr any_string = StringAutomaton::makeAnyString();

  dfaNegation(current_dfa);
  complement_dfa = dfaProduct(any_string->dfa, current_dfa, dfaAND);
  delete any_string;
  any_string = nullptr;
  dfaFree(current_dfa);
  current_dfa = nullptr;

  minimized_dfa = dfaMinimize(complement_dfa);
  dfaFree(complement_dfa);
  complement_dfa = nullptr;

  complement_auto = new StringAutomaton(minimized_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << complement_auto->id << " = [" << this->id << "]->makeComplement()";

  return complement_auto;
}

/**
 * TODO Figure out why empty check is necessary
 */
StringAutomaton_ptr StringAutomaton::union_(StringAutomaton_ptr other_auto) {
  DFA_ptr union_dfa = nullptr, minimized_dfa = nullptr;
  StringAutomaton_ptr union_auto = nullptr;

  union_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaOR);
  minimized_dfa = dfaMinimize(union_dfa);
  dfaFree(union_dfa);
  union_dfa = nullptr;

  //  if ( this->hasEmptyString() || other_auto->hasEmptyString() ) {
  //    tmpM = dfa_union_empty_M(result, var, indices);
  //    dfaFree(result); result = NULL;
  //    result = tmpM;
  //  }

  union_auto = new StringAutomaton(minimized_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << union_auto->id << " = [" << this->id << "]->union(" << other_auto->id << ")";

  return union_auto;
}

StringAutomaton_ptr StringAutomaton::intersect(StringAutomaton_ptr other_auto) {
  DFA_ptr intersect_dfa = nullptr, minimized_dfa = nullptr;
  StringAutomaton_ptr intersect_auto = nullptr;

  intersect_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaAND);
  minimized_dfa = dfaMinimize(intersect_dfa);
  dfaFree(intersect_dfa);
  intersect_dfa = nullptr;

  intersect_auto = new StringAutomaton(minimized_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << intersect_auto->id << " = [" << this->id << "]->intersect(" << other_auto->id << ")";

  return intersect_auto;
}

/**
 * TODO 1 - If we implement this method in low level we can avoid unnecessary
 * dfa product
 * 2 - Before refactoring this try to refactor complement so that item 1- will
 * not be a concern anymore.
 */
StringAutomaton_ptr StringAutomaton::difference(StringAutomaton_ptr other_auto) {
  StringAutomaton_ptr difference_auto = nullptr, complement_auto = nullptr;

  complement_auto = other_auto->complement();
  difference_auto = this->intersect(complement_auto);

  DVLOG(VLOG_LEVEL) << difference_auto->id << " = [" << this->id << "]->difference(" << other_auto->id << ")";

  return difference_auto;
}

StringAutomaton_ptr StringAutomaton::concatenate(StringAutomaton_ptr other_auto) {
  DFA_ptr concat_dfa = nullptr;
  StringAutomaton_ptr concat_auto = nullptr;

//  concat_dfa = dfa_concat(this->dfa, other_auto->dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES,
//          StringAutomaton::DEFAULT_VARIABLE_INDICES);
//

  DFA_ptr M1 = this->dfa;
  DFA_ptr M2 = other_auto->dfa;
  int var = StringAutomaton::DEFAULT_NUM_OF_VARIABLES;
  int* indices = StringAutomaton::DEFAULT_VARIABLE_INDICES;

  StringAutomaton_ptr tmp0 = nullptr;
      DFA *tmp1 = nullptr;

//      if (checkOnlyEmptyString(M1, var, indices)) {
//        return new StringAutomaton(dfaCopy(M2));
//      }
//
//      if (checkOnlyEmptyString(M2, var, indices)) {
//        return new StringAutomaton(dfaCopy(M1));
//      }

      if(checkEmptyString(M2)) {
        if(state_reachable(M2, M2->s, var, indices)){
          tmp1 = dfa_shift_empty_M(M2, var, indices);
          StringAutomaton_ptr tmp = new StringAutomaton(tmp1, var);
          tmp->toDotAscii();
          tmp0 = concat(tmp);
          dfaFree(tmp1);
        }
        else{
          tmp0 =  concat(other_auto);
        }
        tmp1 = dfa_union(tmp0->dfa, M1);
        delete tmp0;
      }else{
        tmp1 = concat(other_auto)->dfa;
      }
  concat_auto = new StringAutomaton(tmp1, num_of_variables);

  DVLOG(VLOG_LEVEL) << concat_auto->id << " = [" << this->id << "]->concatenate(" << other_auto->id << ")";

  return concat_auto;
}

/**
 * Re-implementation of  'dfa_concat_extrabit' in LibStranger
 */
StringAutomaton_ptr StringAutomaton::concat(StringAutomaton_ptr other_auto) {
  DFA_ptr concat_dfa = nullptr, tmp_dfa = nullptr;
  StringAutomaton_ptr concat_auto = nullptr;
  int var = StringAutomaton::DEFAULT_NUM_OF_VARIABLES;
  int* indices = StringAutomaton::DEFAULT_VARIABLE_INDICES;
  int tmp_num_of_variables,
      state_id_shift_amount,
      expected_num_of_states,
      num_of_exceptions_to_add = 0,
      sink_state_left_auto,
      sink_state_right_auto,
      state_key_left_auto = 0,
      state_key_right_auto = 0,
      state_key_fix = 0,
      loc,
      i = 0,
      j = 0;

  long max_exceptions;
  bool is_start_state_reachable;
  paths state_paths = nullptr, pp = nullptr;
  trace_descr tp = nullptr;

  std::map<int, std::vector<char>*> exceptions_left_auto;
  std::map<int, std::vector<char>*> exceptions_right_auto;
  std::map<int, std::vector<char>*> exceptions_fix;
  std::map<int, int> state_map_right_auto;
  std::map<int, int> state_map_left_auto;
  std::map<int, int> state_map_fix;
  char* statuses = nullptr;

  // variable initializations
  sink_state_left_auto = this->getSinkState();
  sink_state_right_auto = other_auto->getSinkState();

  CHECK_GT(sink_state_left_auto, -1);
  CHECK_GT(sink_state_right_auto, -1);

  tmp_num_of_variables = this->num_of_variables + 1; // add one extra bit
  state_id_shift_amount = this->dfa->ns;

  max_exceptions = 1 << tmp_num_of_variables;
  if (tmp_num_of_variables > 10) {
    max_exceptions = 1 << 10; // saving for multi-track automaton
  }

  expected_num_of_states = this->dfa->ns + other_auto->dfa->ns - 1; // -1 is for to remove one of the sink states
  is_start_state_reachable = other_auto->isStartStateReachable();
  if (not is_start_state_reachable) {
    expected_num_of_states = expected_num_of_states  - 1; // if start state is reachable from an accepting state, it will be merge with accepting states of left hand side
  }

  statuses = new char[expected_num_of_states];

//  std::cout << "sink 1: " << sink_state_left_auto << std::endl;
//  std::cout << "sink 2: " << sink_state_right_auto << std::endl;
//  std::cout << "left ns: " << this->dfa->ns << std::endl;
//  std::cout << "right ns: " << other_auto->dfa->ns << std::endl;
//  std::cout << "right initflag: " << is_start_state_reachable << std::endl;
//  std::cout << "new ns: " << expected_num_of_states << std::endl;
//  std::cout << "max exeps: " << max_exceptions << std::endl << std::endl;
//
//  std::cout << "shift: " << state_id_shift_amount << std::endl;
//  std::cout << "right start state: " << other_auto->dfa->s << std::endl;

  dfaSetup(expected_num_of_states, tmp_num_of_variables, getIndices(tmp_num_of_variables)); //sink states are merged
  state_paths = pp = make_paths(other_auto->dfa->bddm, other_auto->dfa->q[other_auto->dfa->s]);
  while (pp) {
    if ( pp->to != sink_state_right_auto ) {
      state_map_right_auto[state_key_right_auto] = pp->to + state_id_shift_amount;
      // if there is a self loop keep it
      if ( pp->to == other_auto->dfa->s ) {
        state_map_right_auto[state_key_right_auto] -= 2;
      } else {
        if ( sink_state_right_auto >= 0 && pp->to > sink_state_right_auto ) {
          state_map_right_auto[state_key_right_auto]--; //to new state, sink state will be eliminated and hence need -1
        }
        if ((not is_start_state_reachable) && pp->to > other_auto->dfa->s) {
          state_map_right_auto[state_key_right_auto]--; // to new state, init state will be eliminated if init is not reachable
        }
      }

      exceptions_right_auto[state_key_right_auto] = new std::vector<char>();
      for (j = 0; j < other_auto->num_of_variables; j++) {
        //the following for loop can be avoided if the indices are in order
        for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) {
            exceptions_right_auto[state_key_right_auto]->push_back('1');
          }
          else {
            exceptions_right_auto[state_key_right_auto]->push_back('0');
          }
        }
        else {
          exceptions_right_auto[state_key_right_auto]->push_back('X');
        }
      }

      exceptions_right_auto[state_key_right_auto]->push_back('1'); // new path
      exceptions_right_auto[state_key_right_auto]->push_back('\0');
      state_key_right_auto++;
    }

    tp = nullptr;
    pp = pp->next;
  }

  kill_paths(state_paths);
  state_paths = pp = nullptr;

  num_of_exceptions_to_add = state_key_right_auto; //num_of_exceptions_to_add is the number of new paths
  for (i = 0; i < this->dfa->ns; i++) {
    state_paths = pp = make_paths(this->dfa->bddm, this->dfa->q[i]);
    state_key_left_auto = 0;
    while (pp) {
      if (pp->to == sink_state_left_auto) {
        pp = pp->next;
        continue;
      }

      state_map_left_auto[state_key_left_auto] = pp->to;
      exceptions_left_auto[state_key_left_auto] = new std::vector<char>();
      for (j = 0; j < this->num_of_variables; j++) {
        for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) {
            exceptions_left_auto[state_key_left_auto]->push_back('1');
          } else {
            exceptions_left_auto[state_key_left_auto]->push_back('0');
          }
        } else {
          exceptions_left_auto[state_key_left_auto]->push_back('X');
        }
      }

      exceptions_left_auto[state_key_left_auto]->push_back('0'); // add extra bit, '0' is used for the exceptions coming from left auto
      exceptions_left_auto[state_key_left_auto]->push_back('\0');

      state_key_left_auto++;
      tp = nullptr;
      pp = pp->next;
    }

    // generate concat automaton
    int num_of_exceptions_left_auto = state_key_left_auto;
    if (this->isAcceptingState(i)) {
      dfaAllocExceptions(num_of_exceptions_left_auto + num_of_exceptions_to_add);
      for (int state_key = 0; state_key < num_of_exceptions_left_auto; state_key++) {
        dfaStoreException(state_map_left_auto[state_key], &*(exceptions_left_auto[state_key]->begin()));
      }
      for (int state_key = 0; state_key < num_of_exceptions_to_add; state_key++) {
        dfaStoreException(state_map_right_auto[state_key], &*(exceptions_right_auto[state_key]->begin()));
      }

      dfaStoreState(sink_state_left_auto);
      if (other_auto->isAcceptingState(0)) {
        statuses[i]='+';
      }
      else {
        statuses[i]='-';
      }
    } else {
      dfaAllocExceptions(num_of_exceptions_left_auto);
      for (int state_key = 0; state_key < num_of_exceptions_left_auto; state_key++) {
        dfaStoreException(state_map_left_auto[state_key], &*exceptions_left_auto[state_key]->begin());
      }
      dfaStoreState(sink_state_left_auto);
      statuses[i] = '-';
    }
    kill_paths(state_paths);
    state_paths = pp = nullptr;
  }

  // clear maps
  for (auto& entry : exceptions_left_auto) {
    entry.second->clear();
    delete entry.second;
  }
  for (auto& entry : exceptions_right_auto) {
    entry.second->clear();
    delete entry.second;
  }
  state_map_left_auto.clear();
  state_map_right_auto.clear();

  //  initflag is 1 iff init is reached by some state. In this case,
  for (i = 0; i < other_auto->dfa->ns; i++) {
    if ( i != sink_state_right_auto ) {
      if ( i != other_auto->dfa->s || is_start_state_reachable) {
        state_paths = pp = make_paths(other_auto->dfa->bddm, other_auto->dfa->q[i]);
        state_key_fix = 0;
        while (pp) {
          if ( pp->to != sink_state_right_auto) {
            state_map_fix[state_key_fix] = pp->to + state_id_shift_amount;

            if ( sink_state_right_auto >= 0 && pp->to > sink_state_right_auto) {
              state_map_fix[state_key_fix]--; //to new state, sink state will be eliminated and hence need -1
            }

            if ( (not is_start_state_reachable) && pp->to > other_auto->dfa->s) {
              state_map_fix[state_key_fix]--; // to new state, init state will be eliminated if init is not reachable
            }

            exceptions_fix[state_key_fix] = new std::vector<char>();
            for (j = 0; j < var; j++) {
              for (tp = pp->trace; tp && (tp->index != indices[j]); tp =tp->next);
              if (tp) {
                if (tp->value){
                  exceptions_fix[state_key_fix]->push_back('1');
                }
                else {
                  exceptions_fix[state_key_fix]->push_back('0');
                }
              }
              else {
                exceptions_fix[state_key_fix]->push_back('X');
              }
            }
            exceptions_fix[state_key_fix]->push_back('0'); // old value
            exceptions_fix[state_key_fix]->push_back('\0');
            state_key_fix++;
            tp = nullptr;
          }
          pp = pp->next;
        }

        dfaAllocExceptions(state_key_fix);
        for (int state_key = 0; state_key < state_key_fix; state_key++) {
          dfaStoreException(state_map_fix[state_key], &*(exceptions_fix[state_key]->begin()));
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

//  statuses[expected_num_of_states]='\0';
  for (auto& entry : exceptions_fix) {
    entry.second->clear();
    delete entry.second;
  }
  state_map_fix.clear();

  concat_dfa = dfaBuild(statuses);
  tmp_dfa = dfaProject(concat_dfa, (unsigned) var);
  delete concat_dfa;
  concat_dfa = dfaMinimize(tmp_dfa);
  delete tmp_dfa;

  concat_auto = new StringAutomaton(concat_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << concat_auto->id << " = [" << this->id << "]->concat(" << other_auto->id << ")";

  return concat_auto;
}

StringAutomaton_ptr StringAutomaton::optional() {
  StringAutomaton_ptr optional_auto = nullptr, empty_string = nullptr;

  empty_string = StringAutomaton::makeEmptyString();
  optional_auto = this->union_(empty_string);
  delete empty_string;

  DVLOG(VLOG_LEVEL) << optional_auto->id << " = [" << this->id << "]->optional()";

  return optional_auto;
}

/**
 * TODO improve implementation by refactoring libstranger call
 *
 */
StringAutomaton_ptr StringAutomaton::closure() {
  DFA_ptr closure_dfa = nullptr;
  StringAutomaton_ptr closure_auto = nullptr;

  closure_dfa = dfa_closure_extrabit(dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES,
          StringAutomaton::DEFAULT_VARIABLE_INDICES);
  closure_auto = new StringAutomaton(closure_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << closure_auto->id << " = [" << this->id << "]->closure()";

  return closure_auto;
}

StringAutomaton_ptr StringAutomaton::kleeneClosure() {
  StringAutomaton_ptr kleene_closure_auto = nullptr, closure_auto = nullptr, empty_string = nullptr;

  closure_auto = this->closure();
  empty_string = StringAutomaton::makeEmptyString();
  kleene_closure_auto = closure_auto->union_(empty_string);
  delete closure_auto;
  delete empty_string;

  DVLOG(VLOG_LEVEL) << kleene_closure_auto->id << " = [" << this->id << "]->kleeneClosure()";

  return kleene_closure_auto;
}


StringAutomaton_ptr StringAutomaton::repeat(unsigned min) {
  StringAutomaton_ptr repeated_auto = nullptr, union_auto = nullptr, concat_auto = nullptr, complement_auto = nullptr,
          closure_auto = nullptr;

  if (min == 0) {
    repeated_auto = this->kleeneClosure();
  } else if (min == 1) {
    repeated_auto = this->closure();
  } else {
    closure_auto = this->closure();
    union_auto = this->clone();
    concat_auto = this->clone();

    for (unsigned int i = 2; i < min; i++) {
      StringAutomaton_ptr temp_concat = concat_auto;
      concat_auto = temp_concat->concatenate(this);
      delete temp_concat;

      StringAutomaton_ptr temp_union = union_auto;
      union_auto = temp_union->union_(concat_auto);
      delete temp_union;
    }

    complement_auto = union_auto->complement();
    repeated_auto = closure_auto->intersect(complement_auto);

    delete complement_auto;
    delete concat_auto;
    delete union_auto;
    delete closure_auto;
  }

  DVLOG(VLOG_LEVEL) << repeated_auto->id << " = [" << this->id << "]->repeat(" << min << ")";

  return repeated_auto;
}

StringAutomaton_ptr StringAutomaton::repeat(unsigned min, unsigned max) {
  StringAutomaton_ptr repeated_auto = nullptr, concat_auto = nullptr;

  // handle min
  if (min == 0) { // {min, max} where min is 0
    repeated_auto = StringAutomaton::makeEmptyString();
    concat_auto = StringAutomaton::makeEmptyString();
  } else {                    // {min, max} where min > 0
    concat_auto = this->clone();     // {min, max} where min = 1
    for (unsigned i = 2; i <= min; i++) { // {min, max} where min > 1
      StringAutomaton_ptr temp_concat = concat_auto;
      concat_auto = temp_concat->concatenate(this);
      delete temp_concat;
    }
    repeated_auto = concat_auto->clone();
  }

  // handle min + 1, max
  for (unsigned i = min + 1; i <= max; i++) {
    StringAutomaton_ptr temp_concat = concat_auto;
    concat_auto = temp_concat->concatenate(this);
    delete temp_concat;

    StringAutomaton_ptr temp_union = repeated_auto;
    repeated_auto = temp_union->union_(concat_auto);
    delete temp_union;
  }

  delete concat_auto;

  DVLOG(VLOG_LEVEL) << repeated_auto->id << " = [" << this->id << "]->repeat(" << min << ", " << max << ")";

  return repeated_auto;
}


StringAutomaton_ptr StringAutomaton::suffixes(){
  DFA_ptr suffix_dfa = nullptr;
  StringAutomaton_ptr suffix_auto = nullptr;

  suffix_dfa = dfaCopy(dfa);

  suffix_auto = new StringAutomaton(dfaMinimize(dfa_Suffix(suffix_dfa,0,0,StringAutomaton::DEFAULT_NUM_OF_VARIABLES,
          StringAutomaton::DEFAULT_VARIABLE_INDICES)), num_of_variables);
  return suffix_auto;
}

StringAutomaton_ptr StringAutomaton::prefixes(){
  DFA_ptr prefix_dfa = nullptr;
  StringAutomaton_ptr prefix_auto = nullptr;
  int sink;

  prefix_dfa = dfaCopy(dfa);
  sink = find_sink(prefix_dfa);

  for (int i = 0; i < prefix_dfa->ns; i++) {
    if(i != sink){
      prefix_dfa->f[i] = 1;
    }
  }

  prefix_auto = new StringAutomaton(dfaMinimize(prefix_dfa), num_of_variables);

  DVLOG(VLOG_LEVEL) << prefix_auto->id << " = [" << this->id << "]->prefixes()";
  return prefix_auto;
}

StringAutomaton_ptr StringAutomaton::suffixesFromIndex(int start){
  DFA_ptr suffix_dfa = nullptr;
  DFA_ptr current_dfa = nullptr;
  StringAutomaton_ptr suffix_auto = nullptr;

  if(start <= 0){
    suffix_dfa = dfaCopy(dfa);
  }
  else{
    suffix_dfa = dfaMinimize(dfa_Suffix(dfaCopy(dfa),start,start,StringAutomaton::DEFAULT_NUM_OF_VARIABLES,
            StringAutomaton::DEFAULT_VARIABLE_INDICES));
  }

  suffix_auto = new StringAutomaton(suffix_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << suffix_auto->id << " = [" << this->id << "]->suffixesFromIndex()";

  return suffix_auto;
}

StringAutomaton_ptr StringAutomaton::prefixesUntilIndex(int index){
  StringAutomaton_ptr prefixes_auto = nullptr;
  StringAutomaton_ptr length_auto = nullptr;
  StringAutomaton_ptr prefixesUntil_auto = nullptr;

  prefixes_auto = this->prefixes();
  length_auto = makeLengthLessThan(index);

  prefixesUntil_auto = prefixes_auto->intersect(length_auto);
  DVLOG(VLOG_LEVEL) << prefixesUntil_auto->id << " = [" << this->id << "]->prefixesUntilIndex("<<index<<")";
  return prefixesUntil_auto;
}

StringAutomaton_ptr StringAutomaton::prefixesAtIndex(int index){
  StringAutomaton_ptr prefixes_auto = nullptr;
  StringAutomaton_ptr length_auto = nullptr;
  StringAutomaton_ptr prefixesAt_auto = nullptr;

  prefixes_auto = this->prefixes();
  prefixesAt_auto = prefixes_auto->intersect(makeLengthEqual(index+1));
  DVLOG(VLOG_LEVEL) << prefixesAt_auto->id << " = [" << this->id << "]->prefixesAtIndex("<<index<<")";
  return prefixesAt_auto;
}

StringAutomaton_ptr StringAutomaton::charAt(int index){
  return this->substring(index,index);
}

StringAutomaton_ptr StringAutomaton::substring(int start){
  StringAutomaton_ptr substring_auto = nullptr;
  substring_auto = this->suffixesFromIndex(start);
  DVLOG(VLOG_LEVEL) << substring_auto->id << " = [" << this->id << "]->substring(" << start << ")";
  return substring_auto;
}

StringAutomaton_ptr StringAutomaton::substring(int start, int end){
  StringAutomaton_ptr substring_auto = nullptr, suffixes_auto = nullptr;
  suffixes_auto = this->suffixesFromIndex(start);
  substring_auto = suffixes_auto->prefixesAtIndex(end - start);
  delete suffixes_auto;
  DVLOG(VLOG_LEVEL) << substring_auto->id << " = [" << this->id << "]->substring(" << start << "," << end << ")";
  return substring_auto;
}


StringAutomaton_ptr StringAutomaton::indexOf(StringAutomaton_ptr search_auto) {

  StringAutomaton_ptr test_auto = nullptr;
  DFA_ptr test = ::dfaSharpStringWithExtraBit(StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  dfaPrintGraphviz(test,StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_UNSIGNED_VARIABLE_INDICES);

  test_auto = StringAutomaton::dfaSharpStringWithExtraBit();
  test_auto->toDot();

  return nullptr;
}

StringAutomaton_ptr StringAutomaton::lastIndexOf(StringAutomaton_ptr search_auto) {
  return nullptr;
}

StringAutomaton_ptr StringAutomaton::contains(StringAutomaton_ptr search_auto) {
  StringAutomaton_ptr contains_auto = nullptr, any_string_auto = nullptr,
          tmp_auto_1 = nullptr, tmp_auto_2 = nullptr;

  any_string_auto = StringAutomaton::makeAnyString();
  tmp_auto_1 = any_string_auto->concatenate(search_auto);
  tmp_auto_2 = tmp_auto_1->concatenate(any_string_auto);

  contains_auto = this->intersect(tmp_auto_2);
  delete any_string_auto;
  delete tmp_auto_1; delete tmp_auto_2;

  DVLOG(VLOG_LEVEL) << contains_auto->id << " = [" << this->id << "]->contains(" << search_auto->id << ")";

  return contains_auto;
}

StringAutomaton_ptr StringAutomaton::begins(StringAutomaton_ptr search_auto) {
  StringAutomaton_ptr begins_auto = nullptr, any_string_auto = nullptr,
          tmp_auto_1 = nullptr;

  any_string_auto = StringAutomaton::makeAnyString();
  tmp_auto_1 = search_auto->concatenate(any_string_auto);

  begins_auto = this->intersect(tmp_auto_1);

  DVLOG(VLOG_LEVEL) << begins_auto->id << " = [" << this->id << "]->begins(" << search_auto->id << ")";

  return begins_auto;
}

StringAutomaton_ptr StringAutomaton::ends(StringAutomaton_ptr search_auto) {
  StringAutomaton_ptr ends_auto = nullptr, any_string_auto = nullptr,
          tmp_auto_1 = nullptr;

  any_string_auto = StringAutomaton::makeAnyString();
  tmp_auto_1 = any_string_auto->concatenate(search_auto);

  ends_auto = this->intersect(tmp_auto_1);

  DVLOG(VLOG_LEVEL) << ends_auto->id << " = [" << this->id << "]->ends(" << search_auto->id << ")";

  return ends_auto;
}

StringAutomaton_ptr StringAutomaton::toUpperCase() {
  DFA_ptr upper_case_dfa = nullptr;
  StringAutomaton_ptr upper_case_auto = nullptr;

  upper_case_dfa = dfaToUpperCase(dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  upper_case_auto = new StringAutomaton(upper_case_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << upper_case_auto->id << " = [" << this->id << "]->toUpperCase()";

  return upper_case_auto;
}

StringAutomaton_ptr StringAutomaton::toLowerCase() {
  DFA_ptr lower_case_dfa = nullptr;
  StringAutomaton_ptr lower_case_auto = nullptr;

  lower_case_dfa = dfaToLowerCase(dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  lower_case_auto = new StringAutomaton(lower_case_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << lower_case_auto->id << " = [" << this->id << "]->toLowerCase()";

  return lower_case_auto;
}

StringAutomaton_ptr StringAutomaton::trim() {
  DFA_ptr trimmed_dfa = nullptr;
  StringAutomaton_ptr trimmed_auto = nullptr;

  trimmed_dfa = dfaTrim(dfa, ' ', StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  trimmed_auto = new StringAutomaton(trimmed_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << trimmed_auto->id << " = [" << this->id << "]->trim()";

  return trimmed_auto;
}

StringAutomaton_ptr StringAutomaton::replace(StringAutomaton_ptr search_auto, StringAutomaton_ptr replace_auto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;

  result_dfa = dfa_general_replace_extrabit(dfa, search_auto->dfa, replace_auto->dfa,
          StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);

  result_auto = new StringAutomaton(result_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->repeat(" << search_auto->id << ", " << replace_auto->id << ")";

  return result_auto;
}

/**
 * TODO Efficiency of the pre image computations can be improved
 * if they are guided by the post image values
 */

StringAutomaton_ptr StringAutomaton::preCharAt(int index, StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr, prefix_auto = nullptr,
      any_string_auto = nullptr, tmp_auto = nullptr;

  prefix_auto = StringAutomaton::makeLengthLessThan(index);
  tmp_auto = prefix_auto->concatenate(this);
  any_string_auto = StringAutomaton::makeAnyString();
  result_auto = tmp_auto->concatenate(any_string_auto);
  delete prefix_auto; delete tmp_auto; delete any_string_auto;

  if (rangeAuto not_eq nullptr) {
    tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preCharAt(" << index << ")";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preSubstring(int start, StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr, prefix_auto = nullptr;

  prefix_auto = StringAutomaton::makeLengthLessThan(start);
  result_auto = prefix_auto->concatenate(this);
  delete prefix_auto;

  if (rangeAuto not_eq nullptr) {
    StringAutomaton_ptr tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preSubstring(" << start << ")";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preSubstring(int start, int end, StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr, prefix_auto = nullptr,
      any_string_auto = nullptr, tmp_auto = nullptr;

  prefix_auto = StringAutomaton::makeLengthLessThan(start);
  tmp_auto = prefix_auto->concatenate(this);
  any_string_auto = StringAutomaton::makeAnyString();
  result_auto = tmp_auto->concatenate(any_string_auto);
  delete prefix_auto; delete tmp_auto; delete any_string_auto;

  if (rangeAuto not_eq nullptr) {
    tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preSubstring(" << start << ", " << end << ")";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preContains() {
  return this->clone();
}

StringAutomaton_ptr StringAutomaton::preBegins() {
  return this->clone();
}

StringAutomaton_ptr StringAutomaton::preEnds() {
  return this->clone();
}

StringAutomaton_ptr StringAutomaton::preToUpperCase(StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;

  result_dfa = dfaPreToUpperCase(dfa,
      StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result_auto = new StringAutomaton(result_dfa, num_of_variables);

  if (rangeAuto not_eq nullptr) {
    StringAutomaton_ptr tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preToUpperCase()";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preToLowerCase(StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;

  result_dfa = dfaPreToLowerCase(dfa,
      StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result_auto = new StringAutomaton(result_dfa, num_of_variables);

  if (rangeAuto not_eq nullptr) {
    StringAutomaton_ptr tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preToLowerCase()";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preTrim(StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;

  result_dfa = dfaPreTrim(dfa, ' ',
      StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result_auto = new StringAutomaton(result_dfa, num_of_variables);

  if (rangeAuto not_eq nullptr) {
    StringAutomaton_ptr tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preTrim()";

  return result_auto;
}

StringAutomaton_ptr StringAutomaton::preReplace(StringAutomaton_ptr searchAuto, std::string replaceString, StringAutomaton_ptr rangeAuto) {
  DFA_ptr result_dfa = nullptr;
  StringAutomaton_ptr result_auto = nullptr;
  std::vector<char> replaceStringVector(replaceString.begin(), replaceString.end());
  replaceStringVector.push_back('\0');

  result_dfa = dfa_pre_replace_str(dfa, searchAuto->dfa, &replaceStringVector[0],
      StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result_auto = new StringAutomaton(result_dfa, num_of_variables);

  if (rangeAuto not_eq nullptr) {
    StringAutomaton_ptr tmp_auto = result_auto;
    result_auto = tmp_auto->intersect(rangeAuto);
    delete tmp_auto;
  }

  DVLOG(VLOG_LEVEL) << result_auto->id << " = [" << this->id << "]->preReplace(" << searchAuto->id << ", " << replaceString << ")";

  return result_auto;
}

/**
 * TODO Needs complete refactoring, has a lot of room for improvements
 * especially in libstranger function calls
 */
bool StringAutomaton::checkEquivalence(StringAutomaton_ptr other_auto) {
  DFA *M[4];
  int result, i;

  M[0] = dfaProduct(this->dfa, other_auto->dfa, dfaIMPL);
  M[1] = dfaProduct(other_auto->dfa, this->dfa, dfaIMPL);
  M[2] = dfa_intersect(M[0], M[1]);
  M[3] = dfa_negate(M[2], StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result = check_emptiness(M[3], StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);

  for (i = 0; i < 4; i++) {
    dfaFree(M[i]);
    M[i] = nullptr;
  }

  return result;
}

/**
 * TODO implement this again independent of libstranger
 */
bool StringAutomaton::isEmptyLanguage() {
  bool result;
  int i = check_emptiness(this->dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  result = (i == 1);
  DVLOG(VLOG_LEVEL) << "[" << this->id << "]->isEmptyLanguage? " << std::boolalpha << result;
  return result;
}

bool StringAutomaton::hasEmptyString() {
  return ((this->dfa->f[this->dfa->s]) == 1) ? true : false;
}

/**
 * TODO Figure out bdd details and find a better way to check that
 * without calling check_equivalence
 */
bool StringAutomaton::isEmptyString() {
  LOG(FATAL)<< "implement me";
  return false;
}

std::string StringAutomaton::getString() {
  char* result = isSingleton(this->dfa,StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES);
  return std::string(result);
}

//void StringAutomaton::toDotAscii(bool print_sink, std::ostream& out) {
//  int sink_status = (print_sink) ? 1 : 0;
//  sink_status = (dfa->ns == 1 and dfa->f[0] == -1) ? 2 : sink_status;
//  dfaPrintGraphvizAsciiRange(dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_VARIABLE_INDICES,
//          sink_status);
//}

/**
 * TODO Refactor lib functions
 *  - find_sink
 *  - ....
 */
void StringAutomaton::toDotAscii(bool print_sink, std::ostream& out) {
  bool force_sink_display = (dfa->ns == 1 and dfa->f[0] == -1) ? true : false;
  paths state_paths, pp;
  trace_descr tp;

  int i, j, k, l, size, maxexp, sink;
  pCharPair *buffer;//array of charpairs references
  char *character;
  pCharPair **toTrans;//array for all states, each entry is an array of charpair references
  int *toTransIndecies;
  char** ranges;

  sink = find_sink(dfa);

  out << "digraph MONA_DFA {\n"
      " rankdir = LR;\n "
      " center = true;\n"
      " size = \"700.5,1000.5\";\n"
      " edge [fontname = Courier];\n"
      " node [height = .5, width = .5];\n"
      " node [shape = doublecircle];";

  for (i = 0; i < dfa->ns; i++) {
    if (dfa->f[i] == 1) {
      out << " " << i << ";";
    }
  }

  out << "\n node [shape = circle];";

  for (i = 0; i < dfa->ns; i++) {
    if (dfa->f[i] == -1) {
      if (i != sink || print_sink) {
        out << " " << i << ";";
      }
    }
  }

  out << "\n node [shape = box];";

  for (i = 0; i < dfa->ns; i++) {
    if (dfa->f[i] == 0) {
      out << " " << i << ";";
    }
  }

  out << "\n init [shape = plaintext, label = \"\"];\n" <<
      " init -> " << dfa->s << ";\n";

  maxexp = 1 << StringAutomaton::DEFAULT_NUM_OF_VARIABLES;
  //TODO convert into c++ style memory management
  buffer = (pCharPair*) malloc(sizeof(pCharPair) * maxexp); //max no of chars from Si to Sj = 2^num_of_variables
  character = (char*) malloc(( StringAutomaton::DEFAULT_NUM_OF_VARIABLES+1) * sizeof(char));
  toTrans = (pCharPair**) malloc(sizeof(pCharPair*) * dfa->ns);//need this to gather all edges out to state Sj from Si
  for (i = 0; i < dfa->ns; i++) {
    toTrans[i] = (pCharPair*) malloc(maxexp * sizeof(pCharPair));
  }
  toTransIndecies = (int*) malloc(dfa->ns * sizeof(int));//for a state Si, how many edges out to each state Sj


  for (i = 0; i < dfa->ns; i++) {
    //get transitions out from state i
    state_paths = pp = make_paths(dfa->bddm, dfa->q[i]);

    //init buffer
    for (j = 0; j < dfa->ns; j++) {
      toTransIndecies[j] = 0;
    }

    for (j = 0; j < maxexp; j++) {
      for (k = 0; k < dfa->ns; k++) {
        toTrans[k][j] = 0;
      }
      buffer[j] = 0;
    }

    //gather transitions out from state i
    //for each transition pp out from state i
    while (pp) {
      if (pp->to == sink && not print_sink){
        pp = pp->next;
        continue;
      }
      //get mona character on transition pp
      for (j = 0; j < StringAutomaton::DEFAULT_NUM_OF_VARIABLES; j++) {
        for (tp = pp->trace; tp && (tp->index != StringAutomaton::DEFAULT_VARIABLE_INDICES[j]); tp = tp->next);

        if (tp) {
          if (tp->value)
            character[j] = '1';
          else
            character[j] = '0';
        } else
          character[j] = 'X';
      }
      character[j] = '\0';
      if (StringAutomaton::DEFAULT_NUM_OF_VARIABLES == 8){
        //break mona character into ranges of ascii chars (example: "0XXX000X" -> [\s-!], [0-1], [@-A], [P-Q])
        size = 0;
        getTransitionChars(character, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, buffer, &size);
        //get current index
        k = toTransIndecies[pp->to];
        //print ranges
        for (l = 0; l < size; l++) {
          toTrans[pp->to][k++] = buffer[l];
          buffer[l] = 0;//do not free just detach
        }
        toTransIndecies[pp->to] = k;
      } else {
//        k = toTransIndecies[pp->to];
//        toTrans[pp->to][k] = (char*) malloc(sizeof(char) * (strlen(character) + 1));
//        strcpy(toTrans[pp->to][k], character);
//        toTransIndecies[pp->to] = k + 1;
      }
      pp = pp->next;
    }

    //print transitions out of state i
    for (j = 0; j < dfa->ns; j++) {
      size = toTransIndecies[j];
      if (size == 0 || (sink == j && not print_sink)) {
        continue;
      }
      ranges = mergeCharRanges(toTrans[j], &size);
      //print edge from i to j
      out << " " << i << " -> " << j << " [label=\"";
      bool print_label = (j != sink || force_sink_display);
      l = 0;//to help breaking into new line
      //for each trans k on char/range from i to j
      for (k = 0; k < size; k++) {
        //print char/range
        if (print_label) {
          out << " " << ranges[k];
        }
        l += strlen(ranges[k]);
        if (l > 18){
          if (print_label) {
            out << "\\n";
          }
          l = 0;
        }
        else if (k < (size - 1)) {
          if (print_label) {
            out << ",";
          }
        }
        free(ranges[k]);
      }//for
      out << "\"];\n";
      if (size > 0)
        free(ranges);
    }
    //for each state free charRange
    //merge with loop above for better performance
    for (j = 0; j < dfa->ns; j++){
      if (j == sink && not print_sink) {
        continue;
      }
      size = toTransIndecies[j];
      for (k = 0; k < size; k++) {
        free(toTrans[j][k]);
      }
    }

    kill_paths(state_paths);
  }//end for each state

  free(character);
  free(buffer);
  for (i = 0; i < dfa->ns; i++){
    free(toTrans[i]);
  }
  free(toTrans);
  free(toTransIndecies);

  out << "}" << std::endl;
}

// TODO will be merge into one toDot function with above
void StringAutomaton::toDot() {
  dfaPrintGraphviz(this->dfa, StringAutomaton::DEFAULT_NUM_OF_VARIABLES, StringAutomaton::DEFAULT_UNSIGNED_VARIABLE_INDICES);
}

void StringAutomaton::printBDD(std::ostream& out) {
  LOG(FATAL) << "implement me, fix headers";
//  Table *table = tableInit();
//  int sink = getSinkState(), i = 0;
//
//  /* remove all marks in a->bddm */
//  bdd_prepare_apply1(this->dfa->bddm);
//
//  /* build table of tuples (idx,lo,hi) */
//  for (i = 0; i < this->dfa->ns; i++) {
//      _export(this->dfa->bddm, this->dfa->q[i], table);
//  }
//
//  /* renumber lo/hi pointers to new table ordering */
//  for (i = 0; i < table->noelems; i++) {
//      if (table->elms[i].idx != -1) {
//          table->elms[i].lo = bdd_mark(this->dfa->bddm, table->elms[i].lo) - 1;
//          table->elms[i].hi = bdd_mark(this->dfa->bddm, table->elms[i].hi) - 1;
//      }
//  }
//
//  out << "digraph MONA_DFA_BDD {\n"
//      "  center = true;\n"
//      "  size = \"100.5,70.5\"\n"
////      "  orientation = landscape;\n"
//      "  node [shape=record];\n"
//      "   s1 [shape=record,label=\"";
//
//  for (i = 0; i < this->dfa->ns; i++) {
//    out << "{" << this->dfa->f[i] << "|<" << i << "> " << i << "}";
//    if (i+1 < table->noelems) {
//      out << "|";
//    }
//  }
//  out << "\"];" << std::endl;
//
//  out << "  node [shape = circle];";
//  for (i = 0; i < table->noelems; i++) {
//    if (table->elms[i].idx != -1) {
//      out << " " << i << " [label=\"" << table->elms[i].idx << "\"];";
//    }
//  }
//
//  out << "\n  node [shape = box];";
//  for (i = 0; i < table->noelems; i++) {
//    if (table->elms[i].idx == -1) {
//      out << " " << i << " [label=\"" << table->elms[i].lo << "\"];";
//    }
//  }
//  out << std::endl;
//
//  for (i = 0; i < this->dfa->ns; i++) {
//    out << " s1:" << i << " -> " << bdd_mark(this->dfa->bddm, this->dfa->q[i]) - 1 << " [style=bold];\n";
//  }
//
//  for (i = 0; i < table->noelems; i++) {
//      if (table->elms[i].idx != -1) {
//          int lo = table->elms[i].lo;
//          int hi = table->elms[i].hi;
//          out << " " << i << " -> " << lo << " [style=dashed];\n";
//          out << " " << i << " -> " << hi << " [style=filled];\n";
//      }
//  }
//  out << "}" << std::endl;
//  tableFree(table);
}

int* StringAutomaton::allocateAscIIIndexWithExtraBit(int length) {
  int* indices = new int[length + 1];
  int i;
  for (i = 0; i <= length; i++) {
    indices[i] = i;
  }
  return indices;
}

int* StringAutomaton::getIndices(int num_of_variables) {
  int* indices = nullptr;
  int i = 0;

  indices = new int[num_of_variables];
  for (i = 0; i < num_of_variables; i++) {
    indices[i] = i;
  }

  return indices;
}

/**
 * That function replaces the getSharp1WithExtraBit and
 * getSharp0WithExtraBit.
 * @return binary representation of reserved word
 */
std::vector<char> StringAutomaton::getReservedWord(char last_char, int length, bool extra_bit) {
  std::vector<char> reserved_word;

  int i;
  for (i = 0; i < length - 1; i++) {
    reserved_word.push_back('1');
  }
  if (extra_bit) {
    reserved_word.push_back('1');
  }
  reserved_word.push_back(last_char);
  reserved_word.push_back('\0');

  return reserved_word;
}

char* StringAutomaton::binaryFormat(unsigned long number, int bit_length) {
  char* binary_str = nullptr;
  int index = bit_length;
  unsigned long subject = number;

  // no extra bit
  binary_str = new char[bit_length + 1];
  binary_str[bit_length] = '\0';

  for (index--; index >= 0; index--) {
    if (subject & 1) {
      binary_str[index] = '1';
    } else {
      binary_str[index] = '0';
    }
    if (subject > 0) {
      subject >>= 1;
    }
  }

  return binary_str;
}

StringAutomaton_ptr StringAutomaton::dfaSharpStringWithExtraBit(int num_of_variables, int* variable_indices) {
  DFA_ptr sharp_string_dfa = nullptr;
  StringAutomaton_ptr sharp_string_extra_bit = nullptr;
  std::array<char, 2> statuses { '-', '+' };
  std::vector<char> reserved_1 = StringAutomaton::getReservedWord('1', num_of_variables, true);
  char* sharp1 = &*reserved_1.begin();

  dfaSetup(2, num_of_variables + 1, variable_indices);
  dfaAllocExceptions(1);
  dfaStoreException(1, sharp1); // word 111111111
  dfaStoreState(0);

  dfaAllocExceptions(0);
  dfaStoreState(1);

  sharp_string_dfa = dfaBuild(&*statuses.begin());
  sharp_string_extra_bit = new StringAutomaton(sharp_string_dfa, num_of_variables);

  DVLOG(VLOG_LEVEL) << sharp_string_extra_bit->id << " = dfaSharpStringWithExtraBit()";

  return sharp_string_extra_bit;
}

bool StringAutomaton::isSinkState(int state_id) {
  return (bdd_is_leaf(this->dfa->bddm, this->dfa->q[state_id])
      and bdd_leaf_value(this->dfa->bddm, this->dfa->q[state_id])
      and this->dfa->f[state_id] == -1);
}

bool StringAutomaton::isAcceptingState(int state_id) {
  return (this->dfa->f[state_id] == 1);
}

/**
 * @returns sink state number if exists, -1 otherwise
 */
int StringAutomaton::getSinkState() {
  for (int i = 0; i < this->dfa->ns; i++) {
    if (isSinkState(i)) {
      return i;
    }
  }
  return -1;
}

/**
 * @returns true if a start state is reachable from an accepting state, false otherwise
 */
bool StringAutomaton::isStartStateReachable() {
  paths state_paths, pp;
  for (int i = 0; i < this->dfa->ns; i++) {
    if (isAcceptingState(i)) {
      state_paths = pp = make_paths(this->dfa->bddm, this->dfa->q[i]);
      while (pp) {
        if (pp->to == this->dfa->s) {
          kill_paths(state_paths);
          return true;
        }
        pp = pp->next;
      }
      kill_paths(state_paths);
    }
  }
  return false;
}

} /* namespace Theory */
} /* namespace Vlab */

