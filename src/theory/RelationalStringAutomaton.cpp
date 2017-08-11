/*
 * RelationalStringAutomaton.cpp
 *
 *  Created on: Jan 23, 2017
 *      Author: baki
 */

#include "RelationalStringAutomaton.h"

namespace Vlab {
namespace Theory {

RelationalStringAutomaton::TransitionTable RelationalStringAutomaton::TRANSITION_TABLE;
const int RelationalStringAutomaton::VLOG_LEVEL = 20;

//RelationalStringAutomaton::RelationalStringAutomaton(DFA_ptr dfa, int num_tracks)
//      : Automaton(Automaton::Type::MULTITRACK, dfa, num_tracks * VAR_PER_TRACK),
//        num_of_tracks_(num_tracks), formula_{nullptr} {
//}

RelationalStringAutomaton::RelationalStringAutomaton(DFA_ptr dfa, StringFormula_ptr formula)
    : Automaton(Automaton::Type::MULTITRACK, dfa, formula->get_number_of_variables() * VAR_PER_TRACK),
      num_of_tracks_(formula->get_number_of_variables()), formula_{formula} {
}

// incoming DFA, in some cases, can have lambda transition already
// need extrabit for nondeterminism in these cases
RelationalStringAutomaton::RelationalStringAutomaton(DFA_ptr dfa, int i_track, int num_tracks, int in_num_vars)
      : Automaton(Automaton::Type::MULTITRACK, nullptr, num_tracks * VAR_PER_TRACK),
        num_of_tracks_(num_tracks), formula_{nullptr} {
  DFA_ptr M = dfa, temp = nullptr, result = nullptr;
  trace_descr tp;
  paths state_paths, pp;
  std::vector<std::pair<std::vector<char>,int>> state_exeps;
  int sink;
  char* statuses;
  int* mindices;
  bool has_sink = true;
  int num_states = M->ns+1; // lambda state
  int lambda_state = num_states-1;
  int var = VAR_PER_TRACK;
  int len = (num_tracks * var)+1; // extrabit for nondeterminism
  mindices = GetBddVariableIndices(len);

  sink = find_sink(M);
  if(sink < 0) {
    has_sink = false;
    sink = num_states;
    num_states++;
  }

  statuses = new char[num_states+1];
  // begin dfa building process
  // old transitions end in '0'
  // new transitions end in '1' (lambda transitions)

  dfaSetup(num_states, len, mindices);
  for(unsigned i = 0; i < M->ns; i++) {
    state_paths = pp = make_paths(M->bddm, M->q[i]);

    // if state is final, add lambda transition to lambda state
    if(M->f[i] == 1) {
      std::vector<char> curr(len,'X');
      for(int k = 0; k < var; k++) {
        curr[i_track+num_tracks*k] = '1';
      }
      curr[len-1] = '1'; // new transition, end with '1'
      curr.push_back('\0');
      state_exeps.push_back(std::make_pair(curr,lambda_state));
    }

    while(pp) {
      if(pp->to != sink) {
        std::vector<char> curr(len,'X');
        for(unsigned j = 0; j < in_num_vars; j++) {
          for(tp = pp->trace; tp && (tp->index != mindices[j]); tp = tp->next);
          if(tp) {
            if(tp->value) curr[i_track+num_tracks*j] = '1';
            else curr[i_track+num_tracks*j] = '0';
          }
          else
            curr[i_track+num_tracks*j] = 'X';
        }
        // if default_num_Var, make default_num_var+1 index '0' for non-lambda
        if(in_num_vars == DEFAULT_NUM_VAR) {
          curr[i_track+num_tracks*(DEFAULT_NUM_VAR)] = '0';
        }
        curr[len-1] = '0'; // old transition, end with '0'
        curr.push_back('\0');
        state_exeps.push_back(std::make_pair(curr,pp->to));
      }
      pp = pp->next;
    }
    kill_paths(state_paths);

    dfaAllocExceptions(state_exeps.size());
    for(unsigned k = 0; k < state_exeps.size(); ++k) {
      dfaStoreException(state_exeps[k].second,&state_exeps[k].first[0]);
    }
    dfaStoreState(sink);
    state_exeps.clear();

    statuses[i] = '-';
  }

  // lambda state, loop de loop
  dfaAllocExceptions(1);
  std::vector<char> str(len,'X');
  for(int i = 0; i < var; i++) {
    str[i_track+num_tracks*i] = '1';
  }
  str[len-1] = '1';
  str.push_back('\0');
  dfaStoreException(lambda_state,&str[0]);
  dfaStoreState(sink);
  statuses[lambda_state] = '+';

  // extra sink state, if needed
  if(!has_sink) {
    dfaAllocExceptions(0);
    dfaStoreState(sink);
    statuses[sink] = '-';
  }

  statuses[num_states] = '\0';
  result = dfaBuild(statuses);
  temp = dfaMinimize(result);
  dfaFree(result);
  // project away the extra bit
  result = dfaProject(temp,len-1);
  dfaFree(temp);
  temp = dfaMinimize(result);
  dfaFree(result);
  result = temp;

  delete[] statuses;
  delete[] mindices;
  this->dfa_ = result;
}

RelationalStringAutomaton::RelationalStringAutomaton(const RelationalStringAutomaton& other)
      : Automaton(other),
        num_of_tracks_(other.num_of_tracks_), formula_ {other.formula_->clone()} {
}

RelationalStringAutomaton::~RelationalStringAutomaton() {
  delete formula_;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::clone() const {
  RelationalStringAutomaton_ptr cloned_auto = new RelationalStringAutomaton(*this);
  return cloned_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::makePrefixSuffix(int left_track, int prefix_track, int suffix_track, int num_tracks) {
//  RelationalStringAutomaton_ptr result_auto = nullptr;
//  DFA_ptr temp_dfa, result_dfa;
//  TransitionVector tv;
//
//  int var = VAR_PER_TRACK;
//  int len = num_tracks * var;
//  int *mindices = getIndices(num_tracks*var);
//  std::vector<char> exep_lambda(var,'1');
//  tv = generate_transitions_for_relation(StringFormula::Type::EQ, var);
//
//  dfaSetup(4,len,mindices);
//  dfaAllocExceptions(2*tv.size() + 1); // 1 extra for lambda stuff below
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for(int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[prefix_track+num_tracks*k] = tv[i].first[k];
//      str[suffix_track+num_tracks*k] = exep_lambda[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(0,&str[0]);
//  }
//
//  // if prefix is lambda, left  and suffix same
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[prefix_track+num_tracks*k] = exep_lambda[k];
//      str[suffix_track+num_tracks*k] = tv[i].first[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(1,&str[0]);
//  }
//
//  // if all 3 are lambda, go to next state
//  std::vector<char> str(len,'X');
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[prefix_track+num_tracks*k] = exep_lambda[k];
//    str[suffix_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // left = suffix, prefix lambda, loop back here
//  dfaAllocExceptions(tv.size() + 1);
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[prefix_track+num_tracks*k] = exep_lambda[k];
//      str[suffix_track+num_tracks*k] = tv[i].first[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(1,&str[0]);
//  }
//  // if all 3 lambda, goto 2
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[prefix_track+num_tracks*k] = exep_lambda[k];
//    str[suffix_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // lambda/lambda state, loop back on lambda
//  dfaAllocExceptions(1);
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // sink
//  dfaAllocExceptions(0);
//  dfaStoreState(3);
//
//  temp_dfa = dfaBuild("--+-");
//  result_dfa = dfaMinimize(temp_dfa);
//  dfaFree(temp_dfa);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//
//  delete[] mindices;
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakePhi(StringFormula_ptr formula) {
  auto non_accepting_dfa = Automaton::DFAMakePhi(formula->get_number_of_variables() * VAR_PER_TRACK);
  auto non_accepting_auto = new RelationalStringAutomaton(non_accepting_dfa, formula);
  return non_accepting_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeAutomaton(StringFormula_ptr formula) {
  RelationalStringAutomaton_ptr result_auto = nullptr;

  switch(formula->get_type()) {
    case StringFormula::Type::EQ:
      result_auto = RelationalStringAutomaton::MakeEquality(formula);
      break;
    case StringFormula::Type::NOTEQ:
      result_auto = RelationalStringAutomaton::MakeNotEquality(formula);
      break;
    case StringFormula::Type::GT:
      result_auto = RelationalStringAutomaton::MakeGreaterThan(formula);
      break;
    case StringFormula::Type::GE:
      result_auto = RelationalStringAutomaton::MakeGreaterThanOrEqual(formula);
      break;
    case StringFormula::Type::LT:
      result_auto = RelationalStringAutomaton::MakeLessThan(formula);
      break;
    case StringFormula::Type::LE:
      result_auto = RelationalStringAutomaton::MakeLessThanOrEqual(formula);
      break;
    case StringFormula::Type::BEGINS:
      result_auto = RelationalStringAutomaton::MakeBegins(formula);
      break;
    case StringFormula::Type::NOTBEGINS:
      result_auto = RelationalStringAutomaton::MakeNotBegins(formula);
      break;
    default:
      LOG(FATAL) << "StringFormula type not supported";
      break;
  }
  return result_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeBegins(StringFormula_ptr formula) {
//  RelationalStringAutomaton_ptr result_auto = nullptr;
//  DFA_ptr temp_dfa, result_dfa;
//  int num_tracks = formula->get_number_of_variables(),
//      left_track,right_track;
//  StringFormula_ptr left_relation = formula->get_left(),
//                     right_relation = formula->get_right();
//  std::string left_data,right_data;
//  TransitionVector tv;
//
//  left_data = left_relation->get_data();
//  right_data = right_relation->get_data();
//  left_track = formula->get_variable_index(left_data);
//  right_track = formula->get_variable_index(right_data);
//
//  int var = VAR_PER_TRACK;
//  int len = num_tracks * var;
//  int *mindices = getIndices(num_tracks*var);
//
//  std::vector<char> exep_lambda(var,'1');
//  tv = generate_transitions_for_relation(StringFormula::Type::EQ,var);
//  dfaSetup(4,len,mindices);
//  dfaAllocExceptions(2*tv.size() + 1); // 1 extra for lambda stuff below
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for(int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = tv[i].second[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(0,&str[0]);
//  }
//
//  // if right is lambda, left can be anything, but go to next state
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = exep_lambda[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(1,&str[0]);
//  }
//
//  // if both are lambda, go to next state
//  std::vector<char> str(len,'X');
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // left anything, right lambda, loop back here
//  dfaAllocExceptions(tv.size()+1);
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = exep_lambda[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(1,&str[0]);
//  }
//  // if both lambda, goto 2
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // lambda/lambda state, loop back on lambda
//  dfaAllocExceptions(1);
//  dfaStoreException(2,&str[0]);
//  dfaStoreState(3);
//
//  // sink
//  dfaAllocExceptions(0);
//  dfaStoreState(3);
//
//  temp_dfa = dfaBuild("--+-");
//  result_dfa = dfaMinimize(temp_dfa);
//  dfaFree(temp_dfa);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  result_auto->setRelation(formula->clone());
//
//  delete[] mindices;
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeNotBegins(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr result_auto = nullptr;
//  DFA_ptr temp_dfa, result_dfa;
//  int num_tracks = relation->get_num_tracks(),
//      left_track,right_track;
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//  std::string left_data,right_data;
//  TransitionVector tv;
//  // "left relation" is variable, "right relation" is constant
//  // the real left is actually just the last track
//  left_data = left_relation->get_data();
//  right_data = right_relation->get_data();
//  left_track = relation->get_variable_index(left_data);
//  right_track = relation->get_variable_index(right_data);
//
//  int eq_eq = 0, lambda_star = 1, not_eq_eq = 2,
//      lambda_lambda = 3, star_lambda = 4, sink = 5;
//  int var = VAR_PER_TRACK;
//  int len = num_tracks * var;
//  int *mindices = getIndices(num_tracks*var);
//  std::vector<char> exep_lambda(var,'1');
//  tv = generate_transitions_for_relation(StringFormula::Type::EQ,var);
//
//  dfaSetup(6,len,mindices);
//  // ------init/eq_eq state
//  // if both the same, and not lambda, loop back
//  dfaAllocExceptions(3*tv.size() + 1); // 1 extra for lambda stuff below
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for(int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = tv[i].second[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(eq_eq,&str[0]);
//  }
//
//  // if left is lambda, right can be anything, but go to lambda_star
//  // but if right is lambda, goto sink
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = exep_lambda[k];
//      str[right_track+num_tracks*k] = tv[i].first[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(lambda_star,&str[0]);
//
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = exep_lambda[k];
//    }
//    dfaStoreException(sink,&str[0]);
//  }
//
//  // if both are lambda, go to sink
//  std::vector<char> str(len,'X');
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(sink,&str[0]);
//  // otherwise, go to not_eq_eq
//  dfaStoreState(not_eq_eq);
//
//
//  // ------ lambda_star state
//  // left lambda, right star, loop back here
//  dfaAllocExceptions(tv.size() + 1);
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = exep_lambda[k];
//      str[right_track+num_tracks*k] = tv[i].first[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(lambda_star,&str[0]);
//  }
//  // if both lambda, goto lambda_lambda
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(lambda_lambda,&str[0]);
//  // otherwise, goto sink
//  dfaStoreState(sink);
//
//
//
//  // ------ not_eq_eq state
//  // on lambda_star goto lambda_star,
//  // star_lambda goto star_lambda,
//  // lambda_lambda goto lambda,
//  // else loop back
//  dfaAllocExceptions(tv.size()*2 + 1);
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = exep_lambda[k];
//      str[right_track+num_tracks*k] = tv[i].first[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(lambda_star,&str[0]);
//
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = exep_lambda[k];
//    }
//    dfaStoreException(star_lambda,&str[0]);
//  }
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(lambda_lambda,&str[0]);
//  dfaStoreState(not_eq_eq);
//
//
//
//
//  // ------ lambda/lambda state, loop back on lambda
//  dfaAllocExceptions(1);
//  dfaStoreException(lambda_lambda,&str[0]);
//  dfaStoreState(sink);
//
//
//
//
//  // ------ star/lambda state
//  // loop back on star/lambda, goto lambda_lambda on lambda/lambda
//  // if right is lambda, left can be anything, but go to next state
//  dfaAllocExceptions(tv.size() + 1);
//  for(int i = 0; i < tv.size(); i++) {
//    std::vector<char> str(len,'X');
//    for (int k = 0; k < var; k++) {
//      str[left_track+num_tracks*k] = tv[i].first[k];
//      str[right_track+num_tracks*k] = exep_lambda[k];
//    }
//    str.push_back('\0');
//    dfaStoreException(star_lambda,&str[0]);
//  }
//
//  // if both are lambda, go to next state
//  str = std::vector<char>(len,'X');
//  for(int k = 0; k < var; k++) {
//    str[left_track+num_tracks*k] = exep_lambda[k];
//    str[right_track+num_tracks*k] = exep_lambda[k];
//  }
//  str.push_back('\0');
//  dfaStoreException(lambda_lambda,&str[0]);
//  dfaStoreState(sink);
//
//  // sink
//  dfaAllocExceptions(0);
//  dfaStoreState(sink);
//
//  temp_dfa = dfaBuild("---+--");
//  result_dfa = dfaMinimize(temp_dfa);
//  dfaFree(temp_dfa);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  result_auto->setRelation(relation->clone());
//
//  delete[] mindices;
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeConcatExtraTrack(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr temp_multi = nullptr, prefix_multi = nullptr,
//                          suffix_multi = nullptr, intersect_multi = nullptr,
//                          result_auto = nullptr;
//  StringAutomaton_ptr any_string = StringAutomaton::makeAnyString(), const_string_auto = nullptr;
//  DFA_ptr temp_dfa = nullptr;
//  int num_tracks = relation->get_num_tracks(),
//      left_track,right_track;
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//
//  std::string left_var_data,const_data;
//
//  left_var_data = left_relation->get_data();
//  const_data = right_relation->get_data();
//
//  // left_track is really just a temp
//  left_track = num_tracks-1;
//  // right_track is var to be concat with, which is actually left_var
//  right_track = relation->get_variable_index(left_var_data);
//
//  if(right_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    const_string_auto = StringAutomaton::makeString(const_data);
//  } else {
//    const_string_auto = StringAutomaton::makeRegexAuto(const_data);
//  }
//
//  temp_dfa = prepend_lambda(const_string_auto->getDFA(),DEFAULT_NUM_VAR);
//  temp_multi = makePrefixSuffix(left_track,right_track,num_tracks,num_tracks+1);
//  prefix_multi = new RelationalStringAutomaton(any_string->getDFA(),right_track,num_tracks+1,DEFAULT_NUM_VAR);
//  suffix_multi = new RelationalStringAutomaton(temp_dfa,num_tracks,num_tracks+1,VAR_PER_TRACK);
//  dfaFree(temp_dfa);
//
//  intersect_multi = temp_multi->intersect(prefix_multi);
//  delete temp_multi;
//  delete prefix_multi;
//
//  temp_multi = intersect_multi;
//  intersect_multi = temp_multi->intersect(suffix_multi);
//  delete temp_multi;
//  delete suffix_multi;
//
//  result_auto = intersect_multi->projectKTrack(num_tracks);
//  delete intersect_multi;
//  delete any_string;
//  delete const_string_auto;
//  result_auto->setRelation(relation->clone());
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeEquality(StringFormula_ptr formula) {
  int num_tracks = formula->get_number_of_variables();
  int left_track = formula->get_variable_index(1); // variable on the left of equality
  int right_track = formula->get_variable_index(2); // variable on the right of equality
  auto result_dfa = make_binary_relation_dfa(StringFormula::Type::EQ, VAR_PER_TRACK, num_tracks, left_track, right_track);
  auto equality_auto = new RelationalStringAutomaton(result_dfa, formula);
  DVLOG(VLOG_LEVEL) << equality_auto->id_ << " = MakeEquality(" << *formula << ")";
  return equality_auto;

}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeNotEquality(StringFormula_ptr formula) {
  int num_tracks = formula->get_number_of_variables();
  int left_track = formula->get_variable_index(1); // variable on the left of disequality
  int right_track = formula->get_variable_index(2); // variable on the right of disequality
  auto result_dfa = make_binary_relation_dfa(StringFormula::Type::NOTEQ, VAR_PER_TRACK, num_tracks, left_track, right_track);
  auto not_equality_auto = new RelationalStringAutomaton(result_dfa, formula);
  DVLOG(VLOG_LEVEL) << not_equality_auto->id_ << " = MakeNotEquality(" << *formula << ")";
  return not_equality_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeLessThan(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
//  StringAutomaton_ptr constant_string_auto = nullptr;
//  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
//
//  std::map<std::string,int> trackmap = relation->get_variable_trackmap();
//  int num_tracks = trackmap.size(),
//      left_track,right_track;
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//  std::string left_data = left_relation->get_data();
//  std::string right_data = right_relation->get_data();
//  if(left_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(left_data);
//  } else if(left_relation->get_type() == StringFormula::Type::REGEX) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
//  } else {
//    left_track = trackmap[left_data];
//  }
//  if(right_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(right_data);
//  } else if(right_relation->get_type() == StringFormula::Type::REGEX) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
//  } else {
//    right_track = trackmap[right_data];
//  }
//  result_dfa = make_binary_relation_dfa(StringFormula::Type::LT,VAR_PER_TRACK,num_tracks,left_track,right_track);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  // if constant_string_auto != nullptr, then either the left or right
//  // side of the inequality is constant; we need to intersect it with
//  // the multitrack where the constant is on the extra track, then
//  // project away the extra track before we return
//  if(constant_string_auto != nullptr) {
//    DVLOG(VLOG_LEVEL) << "NOT EMPTY";
//    RelationalStringAutomaton_ptr constant_multi_auto = new RelationalStringAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
//    temp_auto = result_auto->intersect(constant_multi_auto);
//    delete result_auto;
//    delete constant_multi_auto;
//    delete constant_string_auto;
//    result_auto = temp_auto->projectKTrack(num_tracks-1);
//    delete temp_auto;
//  }
//
//  result_auto->setRelation(relation->clone());
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeLessThanOrEqual(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
//  StringAutomaton_ptr constant_string_auto = nullptr;
//  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
//  std::map<std::string,int> trackmap = relation->get_variable_trackmap();
//  int num_tracks = trackmap.size(),
//      left_track,right_track;
//
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//  std::string left_data = left_relation->get_data();
//  std::string right_data = right_relation->get_data();
//
//  if(left_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(left_data);
//  } else if(left_relation->get_type() == StringFormula::Type::REGEX) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
//  } else {
//    left_track = trackmap[left_data];
//  }
//
//  if(right_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(right_data);
//  } else if(right_relation->get_type() == StringFormula::Type::REGEX) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
//  } else {
//    right_track = trackmap[right_data];
//  }
//
//  result_dfa = make_binary_relation_dfa(StringFormula::Type::LE,VAR_PER_TRACK,num_tracks,left_track,right_track);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  // if constant_string_auto != nullptr, then either the left or right
//  // side of the inequality is constant; we need to intersect it with
//  // the multitrack where the constant is on the extra track, then
//  // project away the extra track before we return
//  if(constant_string_auto != nullptr) {
//    DVLOG(VLOG_LEVEL) << "NOT EMPTY";
//    RelationalStringAutomaton_ptr constant_multi_auto = new RelationalStringAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
//    temp_auto = result_auto->intersect(constant_multi_auto);
//    delete result_auto;
//    delete constant_multi_auto;
//    delete constant_string_auto;
//    result_auto = temp_auto->projectKTrack(num_tracks-1);
//    delete temp_auto;
//  }
//
//  result_auto->setRelation(relation->clone());
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeGreaterThan(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
//  StringAutomaton_ptr constant_string_auto = nullptr;
//  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
//  std::map<std::string,int> trackmap = relation->get_variable_trackmap();
//  int num_tracks = trackmap.size(),
//      left_track,right_track;
//
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//  std::string left_data = left_relation->get_data();
//  std::string right_data = right_relation->get_data();
//
//  if(left_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(left_data);
//  } else if(left_relation->get_type() == StringFormula::Type::REGEX) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
//  } else {
//    left_track = trackmap[left_data];
//  }
//
//  if(right_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(right_data);
//  } else if(right_relation->get_type() == StringFormula::Type::REGEX) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
//  } else {
//    right_track = trackmap[right_data];
//  }
//
//  result_dfa = make_binary_relation_dfa(StringFormula::Type::GT,VAR_PER_TRACK,num_tracks,left_track,right_track);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  // if constant_string_auto != nullptr, then either the left or right
//  // side of the inequality is constant; we need to intersect it with
//  // the multitrack where the constant is on the extra track, then
//  // project away the extra track before we return
//  if(constant_string_auto != nullptr) {
//    DVLOG(VLOG_LEVEL) << "NOT EMPTY";
//    RelationalStringAutomaton_ptr constant_multi_auto = new RelationalStringAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
//    temp_auto = result_auto->intersect(constant_multi_auto);
//    delete result_auto;
//    delete constant_multi_auto;
//    delete constant_string_auto;
//    result_auto = temp_auto->projectKTrack(num_tracks-1);
//    delete temp_auto;
//  }
//
//  result_auto->setRelation(relation->clone());
//
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeGreaterThanOrEqual(StringFormula_ptr relation) {
//  RelationalStringAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
//  StringAutomaton_ptr constant_string_auto = nullptr;
//  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
//  std::map<std::string,int> trackmap = relation->get_variable_trackmap();
//  int num_tracks = trackmap.size(),
//      left_track,right_track;
//
//  StringFormula_ptr left_relation = relation->get_left(),
//                     right_relation = relation->get_right();
//  std::string left_data = left_relation->get_data();
//  std::string right_data = right_relation->get_data();
//
//  if(left_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(left_data);
//  } else if(left_relation->get_type() == StringFormula::Type::REGEX) {
//    left_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
//  } else {
//    left_track = trackmap[left_data];
//  }
//
//  if(right_relation->get_type() == StringFormula::Type::STRING_CONSTANT) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeString(right_data);
//  } else if(right_relation->get_type() == StringFormula::Type::REGEX) {
//    right_track = num_tracks;
//    num_tracks++;
//    constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
//  } else {
//    right_track = trackmap[right_data];
//  }
//
//  result_dfa = make_binary_relation_dfa(StringFormula::Type::GE,VAR_PER_TRACK,num_tracks,left_track,right_track);
//  result_auto = new RelationalStringAutomaton(result_dfa,num_tracks);
//  // if constant_string_auto != nullptr, then either the left or right
//  // side of the inequality is constant; we need to intersect it with
//  // the multitrack where the constant is on the extra track, then
//  // project away the extra track before we return
//  if(constant_string_auto != nullptr) {
//    DVLOG(VLOG_LEVEL) << "NOT EMPTY";
//    RelationalStringAutomaton_ptr constant_multi_auto = new RelationalStringAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
//    temp_auto = result_auto->intersect(constant_multi_auto);
//    delete result_auto;
//    delete constant_multi_auto;
//    delete constant_string_auto;
//    result_auto = temp_auto->projectKTrack(num_tracks-1);
//    delete temp_auto;
//  }
//
//  result_auto->setRelation(relation->clone());
//  return result_auto;
  return nullptr;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeAnyStringUnaligned(StringFormula_ptr formula) {
  DFA_ptr result, temp;
  int len = VAR_PER_TRACK * formula->get_number_of_variables();
  int *mindices = Automaton::GetBddVariableIndices(len);

  dfaSetup(1, len, mindices);
  dfaAllocExceptions(0);
  dfaStoreState(0);

  temp = dfaBuild("+");
  result = dfaMinimize(temp);
  dfaFree(temp);
  delete[] mindices;
  return new RelationalStringAutomaton(result, formula);
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::MakeAnyStringAligned(StringFormula_ptr formula) {
  RelationalStringAutomaton_ptr aligned_auto = nullptr, any_auto = nullptr, temp_auto = nullptr;
  StringAutomaton_ptr any_string_auto = nullptr;

  aligned_auto = MakeAnyStringUnaligned(formula->clone());
  any_string_auto = StringAutomaton::MakeAnyString();
  const int number_of_string_vars = formula->get_number_of_variables();
  for(unsigned i = 0; i < number_of_string_vars; i++) {
    any_auto = new RelationalStringAutomaton(any_string_auto->getDFA(), i, number_of_string_vars);
    temp_auto = aligned_auto->Intersect(any_auto);
    delete aligned_auto;
    delete any_auto;
    aligned_auto = temp_auto;
  }
  delete any_string_auto;
  return aligned_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::Complement() {
  DFA_ptr complement_dfa = dfaCopy(this->dfa_);
  dfaNegation(complement_dfa);
  auto temp_auto = new RelationalStringAutomaton(complement_dfa, formula_->negate());
  auto aligned_universe_auto = MakeAnyStringAligned(formula_->clone());
  auto complement_auto = temp_auto->Intersect(aligned_universe_auto);
  delete temp_auto;
  delete aligned_universe_auto;
  DVLOG(VLOG_LEVEL) << complement_auto->id_ << " = [" << this->id_ << "]->Complement()";
  return complement_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::Union(RelationalStringAutomaton_ptr other_auto) {
  if (this->formula_->get_number_of_variables() != other_auto->formula_->get_number_of_variables()) {
    LOG(FATAL) << "Number of variables are not equal!";
    return nullptr;
  }

  auto union_dfa = Automaton::DFAUnion(this->dfa_,other_auto->dfa_);
  auto formula = formula_->clone();
  formula->reset_param_orders();
  auto union_auto = new RelationalStringAutomaton(union_dfa, formula);
  DVLOG(VLOG_LEVEL) << union_auto->id_ << " = [" << this->id_ << "]->Union(" << other_auto->id_ << ")";
  return union_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::Difference(RelationalStringAutomaton_ptr other_auto) {
  if (this->formula_->get_number_of_variables() != other_auto->formula_->get_number_of_variables()) {
    LOG(FATAL) << "Number of variables are not equal!";
    return nullptr;
  }

  auto complement_auto = other_auto->Complement();
  auto difference_auto = this->Intersect(complement_auto);
  delete complement_auto;
  DVLOG(VLOG_LEVEL) << complement_auto->id_ << " = [" << this->id_ << "]->Difference(" << other_auto->id_ << ")";
  return difference_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::Intersect(RelationalStringAutomaton_ptr other_auto) {
  if (this->formula_->get_number_of_variables() != other_auto->formula_->get_number_of_variables()) {
    LOG(FATAL) << "Number of variables are not equal!";
    return nullptr;
  }

  auto intersect_dfa = DFAIntersect(this->dfa_,other_auto->dfa_);
  auto formula = formula_->clone();
  formula->reset_param_orders();
  auto intersect_auto = new RelationalStringAutomaton(intersect_dfa, formula);
  DVLOG(VLOG_LEVEL) << intersect_auto->id_ << " = [" << this->id_ << "]->Intersect(" << other_auto->id_ << ")";

  return intersect_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::Intersect(StringAutomaton_ptr other_auto) {
  if (other_auto->get_formula() == nullptr) {
    LOG(FATAL) << "Single track automaton needs to have formula";
    return nullptr;
  }
  std::string var_name = other_auto->get_formula()->get_variable_at_index(0);
  auto rel_other_auto = new RelationalStringAutomaton(dfaCopy(other_auto->getDFA()), formula_->get_variable_index(var_name), formula_->get_number_of_variables());
  rel_other_auto->formula_ = formula_->clone();

  auto intersect_auto = this->Intersect(rel_other_auto);
  delete rel_other_auto;
  //  DVLOG(VLOG_LEVEL) << intersect_auto->id_ << " = [" << this->id_ << "]->Intersect(" << other_auto->id_ << ")";
  return intersect_auto;
}

/**
 * TODO be careful with the number of variables in the formula and the number of tracks
 */
RelationalStringAutomaton_ptr RelationalStringAutomaton::ProjecAwayVariable(std::string var_name) {
  int index = formula_->get_variable_index(var_name);

  auto result_auto = ProjectKTrack(index);
  result_auto->formula_->remove_variable(var_name);

  DVLOG(VLOG_LEVEL) << result_auto->id_ << " = [" << this->id_ << "]->ProjecAwayVariable(" << var_name << ")";
  return result_auto;
}

RelationalStringAutomaton_ptr RelationalStringAutomaton::ProjectKTrack(int k_track) {
  RelationalStringAutomaton_ptr result_auto;
  DFA_ptr temp,result_dfa = this->dfa_;
  int flag = 0;
  int *map = GetBddVariableIndices(this->num_of_tracks_*VAR_PER_TRACK);
  for(int i = 0,k=0,l=0; i < this->num_of_bdd_variables_; i++) {
      if(i == k_track+l*this->num_of_tracks_) {
          map[i] = (this->num_of_tracks_-1)*VAR_PER_TRACK+l;
          l++;
          continue;
      }
      map[i] = k++;
  }
  for(unsigned j = 0; j < VAR_PER_TRACK; j++) {
    temp = dfaProject(result_dfa,k_track+this->num_of_tracks_*j);
    if(flag)
      dfaFree(result_dfa);
    result_dfa = dfaMinimize(temp);
    flag = 1;
    dfaFree(temp);
  }
  dfaReplaceIndices(result_dfa,map);
  delete[] map;
  result_auto = new RelationalStringAutomaton(result_dfa, formula_->clone());
  result_auto->num_of_tracks_ = result_auto->num_of_tracks_ - 1;
  DVLOG(VLOG_LEVEL) << result_auto->id_ << " = [" << this->id_ << "]->ProjectKTrack(" << k_track << ")";
  return result_auto;
}

StringAutomaton_ptr RelationalStringAutomaton::GetAutomatonForVariable(std::string var_name) {
  int index = formula_->get_variable_index(var_name);
  auto result_auto = GetKTrack(index);
  DVLOG(VLOG_LEVEL) << result_auto->getId() << " = [" << this->id_ << "]->GetAutomatonForVariable(" << var_name << ")";
  return result_auto;
}

StringAutomaton_ptr RelationalStringAutomaton::GetKTrack(int k_track) {
  DFA_ptr result = this->dfa_, temp;
  StringAutomaton_ptr result_auto = nullptr;
  int flag = 0;

  if(k_track >= this->num_of_tracks_) {
    LOG(FATAL) << "error in RelationalStringAutomaton::getKTrack; k_track,num_tracks = " << k_track << "," << this->num_of_tracks_;
  } else if(this->num_of_tracks_ == 1) {
    // TODO baki: better handle this situation where mixed constraint and multi-track really get mixed
//    DVLOG(VLOG_LEVEL) << "   getKTrack, but only 1 track";
//    result = trim_lambda_suffix(this->dfa_,this->num_of_variables_);

//    result_auto = new StringAutomaton(result);


    // TODO baki: added below for charat example
    temp = dfaProject(result, this->num_of_bdd_variables_ - 1);
    result = dfaMinimize(temp);
    dfaFree(temp);
    result_auto = new StringAutomaton(result);

    return result_auto;
  }

    // k_track needs to be mapped to indices 0-VAR_PER_TRACK
    // while all others need to be pushed back by VAR_PER_TRACK, then
    // interleaved with 1 less than current number of tracks

  int* map = GetBddVariableIndices(this->num_of_tracks_*VAR_PER_TRACK);
  for(int i = 0; i < this->num_of_tracks_; i++) {
    if(i == k_track) {
      for(int k = 0; k < VAR_PER_TRACK; k++) {
        map[i+this->num_of_tracks_*k] = k;
      }
    } else {
      for(int k = 0; k < VAR_PER_TRACK; k++) {
        map[i+this->num_of_tracks_*k] = VAR_PER_TRACK + i+(this->num_of_tracks_-1)*k;
      }
    }
  }

  for(int i = this->num_of_tracks_-1; i >= 0; --i) {
    if(i != k_track) {
      for(int j = 0; j < VAR_PER_TRACK; ++j) {
        temp = dfaProject(result,(unsigned)(i+this->num_of_tracks_*j));
        if(flag)
          dfaFree(result);
        result = dfaMinimize(temp);
        flag = 1;
        dfaFree(temp);
      }
    }
  }
  dfaReplaceIndices(result,map);
  delete[] map;

  if(find_sink(result) != -1) {
    // trim prefix first, then suffix
    temp = trim_lambda_suffix(result,VAR_PER_TRACK,false);

    dfaFree(result);
    result = temp;

    temp = trim_lambda_prefix(result, VAR_PER_TRACK);
    dfaFree(result);
    result = temp;
    result_auto = new StringAutomaton(result);
  } else {
    DVLOG(VLOG_LEVEL) << "no sink";
    dfaFree(result);
    result_auto = StringAutomaton::MakeAnyString();
  }
  return result_auto;
}

void RelationalStringAutomaton::SetSymbolicCounter() {
  // remove last lambda loop
  DFA_ptr original_dfa = nullptr, temp_dfa = nullptr,trimmed_dfa = nullptr;
  original_dfa = this->dfa_;
  trace_descr tp;
  paths state_paths,pp;
  int sink = find_sink(original_dfa);
  if(sink < 0) {
    LOG(FATAL) << "Cant count, no sink!";
  }
  int var = VAR_PER_TRACK;
  int len = var * num_of_tracks_;
  int* mindices = GetBddVariableIndices(len);
  char* statuses = new char[original_dfa->ns+1];
  std::vector<std::pair<std::vector<char>,int>> state_exeps;
  std::vector<bool> lambda_states(original_dfa->ns,false);
  dfaSetup(original_dfa->ns,len,mindices);
  for(int i = 0; i < original_dfa->ns; i++) {
    statuses[i] = '-';
    state_paths = pp = make_paths(original_dfa->bddm, original_dfa->q[i]);
    while(pp) {
      if(pp->to == sink) {
        pp = pp->next;
        continue;
      }

      std::vector<char> exep(len,'X');
      for(int j = 0; j < len; j++) {
        for(tp = pp->trace; tp && (tp->index != mindices[j]); tp= tp->next);
        if(tp) {
          if(tp->value) exep[j] = '1';
          else exep[j] = '0';
        }
        else exep[j] = 'X';
      }

      // if lambda and loops back, dont add it
      bool is_lambda = true;
      for(int k = 0; k < len; k++) {
        if(exep[k] != '1' && exep[k] != 'X') {
          is_lambda = false;
          break;
        }
      }

      if(is_lambda) {
        lambda_states[pp->to] = true;
        if(!lambda_states[i] || i == pp->to) {
          statuses[i] = '+';
        }
      } else {
        exep.push_back('\0');
        state_exeps.push_back(std::make_pair(exep,pp->to));
      }
      pp = pp->next;
    }
    kill_paths(state_paths);
    dfaAllocExceptions(state_exeps.size());
    for(int k = 0; k < state_exeps.size(); k++) {
      dfaStoreException(state_exeps[k].second, &state_exeps[k].first[0]);
    }
    dfaStoreState(sink);
    state_exeps.clear();
  }
  statuses[original_dfa->ns] = '\0';
  temp_dfa = dfaBuild(statuses);
  trimmed_dfa = dfaMinimize(temp_dfa);
  dfaFree(temp_dfa);
  delete[] mindices;
  delete[] statuses;

  this->dfa_ = trimmed_dfa;
  Automaton::SetSymbolicCounter();
  this->dfa_ = original_dfa;
  dfaFree(trimmed_dfa);
}

std::vector<std::string> RelationalStringAutomaton::getAnAcceptingStringForEachTrack() {
  std::vector<std::string> strings(num_of_tracks_, "");
  std::vector<bool>* example = getAnAcceptingWord();
  unsigned char c = 0;
  unsigned num_transitions = example->size() / num_of_bdd_variables_;
  bool bit;
  unsigned sharp1 = 254, sharp2 = 255;

  for(int t = 0; t < num_transitions; t++) {
    unsigned offset = t*num_of_bdd_variables_;
    for (int i = 0; i < num_of_tracks_; i++) {
      for (int j = 0; j < VAR_PER_TRACK; j++) {
        bit = (*example)[offset+i+num_of_tracks_*j];
        if(bit) {
          c |= 1;
        } else {
          c |= 0;
        }
        if(j != VAR_PER_TRACK-1) {
          c <<= 1;
        }
      }
      if(c != sharp1 && c != sharp2) strings[i] += c;
      c = 0;
    }
  }
  delete example;
  return strings;
}

int RelationalStringAutomaton::getNumTracks() const {
  return this->num_of_tracks_;
}

StringFormula_ptr RelationalStringAutomaton::get_formula() {
  return formula_;
}

void RelationalStringAutomaton::set_formula(StringFormula_ptr formula) {
  this->formula_ = formula;
}

const RelationalStringAutomaton::TransitionVector& RelationalStringAutomaton::generate_transitions_for_relation(StringFormula::Type type, int bits_per_var) {
  bits_per_var--;
  // check table for precomputed value first
  std::pair<int,StringFormula::Type> key(bits_per_var,type);
  if(TRANSITION_TABLE.find(key) != TRANSITION_TABLE.end()) {
    return TRANSITION_TABLE[key];
  }

  // not previously computed; compute now and cache for later.
  bool final_states[3] = {false,false,false};
  switch(type) {
    case StringFormula::Type::EQ:
      final_states[0] = true;
      break;
    case StringFormula::Type::NOTEQ:
      final_states[1] = true;
      final_states[2] = true;
      break;
    case StringFormula::Type::LT:
      final_states[1] = true;
      break;
    case StringFormula::Type::LE:
      final_states[0] = true;
      final_states[1] = true;
      break;
    case StringFormula::Type::GT:
      final_states[2] = true;
      break;
    case StringFormula::Type::GE:
      final_states[0] = true;
      final_states[2] = true;
      break;
    default:
      LOG(FATAL) << "Invalid relation ordering type";
      break;
  }

  std::vector<std::map<std::string,int>> states(3, std::map<std::string, int>());
  states[0]["00"] = 0;
  states[0]["01"] = 1;
  states[0]["10"] = 2;
  states[0]["11"] = 0;

  states[1]["XX"] = 1;
  states[2]["XX"] = 2;

  TransitionVector good_trans;
  std::queue<std::pair<int,std::pair<std::string,std::string>>> next;
  next.push(std::make_pair(0,std::make_pair("","")));

  while(!next.empty()) {
    std::pair<int,std::pair<std::string,std::string>> curr = next.front();
    if(curr.second.first.size() >= bits_per_var) {
      if(final_states[curr.first]) {
        // append lambda bit for multitrack lambda
        curr.second.first += '0';
        curr.second.second += '0';
        good_trans.push_back(curr.second);
      }
    } else {
      for(auto& t : states[curr.first]) {
        next.push(std::make_pair(
            t.second,
            std::make_pair(
                curr.second.first + std::string(1,t.first[0]),
                curr.second.second + std::string(1,t.first[1]))));
      }
    }
    next.pop();
  }

  // cache the transitions for later
  TRANSITION_TABLE[key] = good_trans;

  return TRANSITION_TABLE[key];
}

DFA_ptr RelationalStringAutomaton::make_binary_relation_dfa(StringFormula::Type type, int bits_per_var, int num_tracks, int left_track, int right_track) {
  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, aligned_dfa = nullptr;
  int var = bits_per_var;
  int len = num_tracks * var;
  int *mindices = GetBddVariableIndices(num_tracks*var);
  int eq = 0,
      left = 1,
      right = 2,
      sink = 3;
  char statuses[5] = {"----"};
  std::vector<std::pair<std::vector<char>,int>> exeps;
  TransitionVector tv_eq, tv_left, tv_right;
  tv_eq = generate_transitions_for_relation(StringFormula::Type::EQ,bits_per_var);
  tv_left = generate_transitions_for_relation(StringFormula::Type::LT,bits_per_var);
  tv_right = generate_transitions_for_relation(StringFormula::Type::GT,bits_per_var);

  switch(type) {
    case StringFormula::Type::EQ:
      statuses[eq] = '+';
      break;
    case StringFormula::Type::NOTEQ:
      statuses[left] = '+';
      statuses[right] = '+';
      break;
    case StringFormula::Type::LT:
      statuses[left] = '+';
      break;
    case StringFormula::Type::LE:
      statuses[eq] = '+';
      statuses[left] = '+';
      break;
    case StringFormula::Type::GT:
      statuses[right] = '+';
      break;
    case StringFormula::Type::GE:
      statuses[eq] = '+';
      statuses[right] = '+';
      break;
    default:
      DVLOG(VLOG_LEVEL) << "Invalid stringrelation type! can't make dfa...";
      delete mindices;
      return nullptr;
  }

  for(auto& t : tv_eq) {
    std::vector<char> str(len,'X');
    for(int k = 0; k < var; k++) {
      str[left_track+num_tracks*k] = t.first[k];
      str[right_track+num_tracks*k] = t.second[k];
    }
    str.push_back('\0');
    exeps.push_back(std::make_pair(str,eq));
  }

  for(auto& t : tv_left) {
    std::vector<char> str(len,'X');
    for(int k = 0; k < var; k++) {
      str[left_track+num_tracks*k] = t.first[k];
      str[right_track+num_tracks*k] = t.second[k];
    }
    str.push_back('\0');
    exeps.push_back(std::make_pair(str,left));
  }

  for(auto& t : tv_right) {
    std::vector<char> str(len,'X');
    for(int k = 0; k < var; k++) {
      str[left_track+num_tracks*k] = t.first[k];
      str[right_track+num_tracks*k] = t.second[k];
    }
    str.push_back('\0');
    exeps.push_back(std::make_pair(str,right));
  }

  std::vector<char> str(len,'X');
  for(int k = 0; k < var; k++) {
    str[left_track+num_tracks*k] = '1';
    str[right_track+num_tracks*k] = '1';
  }
  str.push_back('\0');
  exeps.push_back(std::make_pair(str,eq));

  for(int k = 0; k < var-1; k++) {
    str[left_track + num_tracks * k] = 'X';
    str[right_track+ num_tracks * k] = '1';
  }
  str[left_track+num_tracks*(var-1)] = '0';
  str[right_track+num_tracks*(var-1)] = '1';
  exeps.push_back(std::make_pair(str,right));

  for(int k = 0; k < var-1; k++) {
    str[left_track + num_tracks * k] = '1';
    str[right_track+ num_tracks * k] = 'X';
  }
  str[left_track+num_tracks*(var-1)] = '1';
  str[right_track+num_tracks*(var-1)] = '0';
  exeps.push_back(std::make_pair(str,left));


  dfaSetup(4,len,mindices);
  dfaAllocExceptions(exeps.size());
  for(int i = 0; i < exeps.size(); i++) {
    dfaStoreException(exeps[i].second, &(exeps[i].first)[0]);
  }
  dfaStoreState(sink);
  exeps.clear();

  dfaAllocExceptions(0);
  dfaStoreState(left);

  dfaAllocExceptions(0);
  dfaStoreState(right);

  // sink
  dfaAllocExceptions(0);
  dfaStoreState(sink);

  // build it!
  temp_dfa = dfaBuild(statuses);
  result_dfa = dfaMinimize(temp_dfa);
  dfaFree(temp_dfa);

  aligned_dfa = make_binary_aligned_dfa(left_track,right_track,num_tracks);
  temp_dfa = dfaProduct(result_dfa,aligned_dfa,dfaAND);

  dfaFree(result_dfa);
  result_dfa = dfaMinimize(temp_dfa);
  dfaFree(temp_dfa);
  dfaFree(aligned_dfa);

  delete mindices;
  return result_dfa;
}

DFA_ptr RelationalStringAutomaton::make_binary_aligned_dfa(int left_track, int right_track, int num_tracks) {
  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr;
  TransitionVector tv;
  int init = 0,lambda_star = 1, lambda_lambda = 2,
      star_lambda = 3, sink = 4;
  int var = VAR_PER_TRACK;
  int len = num_tracks * var;
  int *mindices = GetBddVariableIndices(num_tracks*var);
  std::vector<char> exep_lambda(var,'1');
  std::vector<char> exep_dont_care(var,'X');
  exep_dont_care[var-1] = '0';
  tv = generate_transitions_for_relation(StringFormula::Type::EQ,var);

  dfaSetup(5,len,mindices);

  // ---- init state
  // if lambda/star goto lambda_star,
  // if star/lambda goto star_lambda,
  // if lambda_lambda goto lambda_lambda,
  // if star/star, loop
  // else, sink

  std::vector<char> str(len,'X');
  str.push_back('\0');

  dfaAllocExceptions(4);
  // x,x
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_dont_care[i];
    str[right_track+num_tracks*i] = exep_dont_care[i];
  }
  dfaStoreException(init, &str[0]);

  // x,lambda
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_dont_care[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(star_lambda, &str[0]);

  // lambda,x
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_dont_care[i];
  }
  dfaStoreException(lambda_star, &str[0]);

  //lambda,lambda
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(lambda_lambda, &str[0]);
  dfaStoreState(sink);

  // ---- lambda_star state ----

  dfaAllocExceptions(2);
  // lambda,x
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_dont_care[i];
  }
  dfaStoreException(lambda_star, &str[0]);
  //lambda,lambda
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(lambda_lambda, &str[0]);
  dfaStoreState(sink);

  // ---- lambda_lambda state ----

  dfaAllocExceptions(1);
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(lambda_lambda, &str[0]);
  dfaStoreState(sink);

  // ---- star_lambda state ----

  dfaAllocExceptions(2);
  // lambda,x
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_dont_care[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(star_lambda, &str[0]);
  //lambda,lambda
  for(int i = 0; i < var; i++) {
    str[left_track+num_tracks*i] = exep_lambda[i];
    str[right_track+num_tracks*i] = exep_lambda[i];
  }
  dfaStoreException(lambda_lambda, &str[0]);
  dfaStoreState(sink);

  // ---- sink state -----

  dfaAllocExceptions(0);
  dfaStoreState(sink);

  temp_dfa = dfaBuild("--+--");
  result_dfa = dfaMinimize(temp_dfa);
  dfaFree(temp_dfa);

  delete[] mindices;
  return result_dfa;
}

bool RelationalStringAutomaton::is_exep_equal_char(std::vector<char> exep, std::vector<char> cvec, int var) {
  for(int i = 0; i < var; i++) {
    if(exep[i] != cvec[i])
      return false;
  }
  return true;
}

bool RelationalStringAutomaton::is_exep_include_char(std::vector<char> exep, std::vector<char> cvec, int var) {
  for(int i = 0; i < var; i++) {
    if(exep[i] != 'X' && exep[i] != cvec[i])
      return false;
  }
  return true;
}

// resulting dfa has 1 more bit for lambda stuff
DFA_ptr RelationalStringAutomaton::prepend_lambda(DFA_ptr dfa, int var) {
  if(var != DEFAULT_NUM_VAR) {
    LOG(FATAL) << "mismatched incoming var";
  }
  DFA_ptr M = dfa, temp = nullptr, result = nullptr;
  trace_descr tp;
  paths state_paths, pp;
  std::vector<std::pair<std::vector<char>,int>> state_exeps;
  int num_states = M->ns+1;
  int sink = Automaton::find_sink(M);
  bool has_sink = true;

  if(sink < 0) {
    has_sink = false;
    sink = num_states;
    num_states++;
  } else {
    sink++; // +1 for new state
  }

  char* statuses;
  int* mindices;
  int len = VAR_PER_TRACK; // 1 more than default_num_var

  mindices = GetBddVariableIndices(len);
  statuses = new char[num_states+1];

  // begin dfa building process
  dfaSetup(num_states, len, mindices);

  // setup for initial state
  state_paths = pp = make_paths(M->bddm, M->q[0]);
  while(pp) {
    if(pp->to != sink-1) {
      std::vector<char> curr(len,'0');
      for(unsigned j = 0; j < var; j++) {
        for(tp = pp->trace; tp && (tp->index != mindices[j]); tp = tp->next);
        if(tp) {
          if(tp->value) curr[j] = '1';
          else curr[j] = '0';
        }
        else
          curr[j] = 'X';
      }
      curr.push_back('\0');
      state_exeps.push_back(std::make_pair(curr,pp->to+1));
    }
    pp = pp->next;
  }
  kill_paths(state_paths);

  // add lambda loop to self
  std::vector<char> str(len,'1');
  str.push_back('\0');
  state_exeps.push_back(std::make_pair(str,0));
  dfaAllocExceptions(state_exeps.size());
  for(unsigned k = 0; k < state_exeps.size(); ++k) {
    dfaStoreException(state_exeps[k].second,&state_exeps[k].first[0]);
  }
  dfaStoreState(sink);

  state_exeps.clear();
  if(M->f[0] == 1) {
    statuses[0] = '+';
  } else {
    statuses[0] = '-';
  }

  // rest of states (shift 1)
  for(unsigned i = 0; i < M->ns; i++) {
    state_paths = pp = make_paths(M->bddm, M->q[i]);

    while(pp) {
      if(pp->to != sink-1) {
        std::vector<char> curr(len,'0');
        for(unsigned j = 0; j < var; j++) {
          for(tp = pp->trace; tp && (tp->index != mindices[j]); tp = tp->next);
          if(tp) {
            if(tp->value) curr[j] = '1';
            else curr[j] = '0';
          }
          else
            curr[j] = 'X';
        }
        curr.push_back('\0');
        state_exeps.push_back(std::make_pair(curr,pp->to+1));
      }
      pp = pp->next;
    }
    kill_paths(state_paths);

    dfaAllocExceptions(state_exeps.size());
    for(unsigned k = 0; k < state_exeps.size(); ++k) {
      dfaStoreException(state_exeps[k].second,&state_exeps[k].first[0]);
    }
    dfaStoreState(sink);
    state_exeps.clear();

    if(M->f[i] == 1) {
      statuses[i+1] = '+';
    } else if(M->f[i] == -1) {
      statuses[i+1] = '-';
    } else {
      statuses[i+1] = '0';
    }
  }

  if(!has_sink) {
    dfaAllocExceptions(0);
    dfaStoreState(sink);
    statuses[sink] = '-';
  }

  statuses[num_states] = '\0';
  temp = dfaBuild(statuses);
  result = dfaMinimize(temp);
  dfaFree(temp);

  delete[] statuses;
  delete[] mindices;

  return result;
}

// incoming dfa has extrabit for lambda
// remove lambda transitions and project away extra bit
DFA_ptr RelationalStringAutomaton::trim_lambda_prefix(DFA_ptr dfa, int var, bool project_bit) {
  if(var != VAR_PER_TRACK) {
    LOG(FATAL) << "not correct var";
  }
  DFA_ptr result_dfa = nullptr, temp_dfa = nullptr;
  paths state_paths, pp;
  trace_descr tp;
  char* statuses;
  int *indices = Automaton::GetBddVariableIndices(var);
  int sink = find_sink(dfa);
  CHECK_GT(sink,-1);
  std::vector<char> lambda_vec(var,'1');
  // start at start-state
  // if transition is lambda, we need to add that "to" state to the
  // pool of possible start states
  std::vector<bool> states_visited(dfa->ns,false);
  std::vector<int> reachable;
  std::queue<int> states_to_visit;

  states_to_visit.push(dfa->s);
  states_visited[dfa->s] = true;
  reachable.push_back(dfa->s);

  while(!states_to_visit.empty()) {
    int state = states_to_visit.front();
    states_to_visit.pop();
    state_paths = pp = make_paths(dfa->bddm, dfa->q[state]);
    std::vector<char> exep(var,'X');
    while(pp) {
      if(pp->to == sink) {
        pp = pp->next;
        continue;
      }

      for(int j = 0; j < var; j++) {
        for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) exep[j] = '1';
          else exep[j] = '0';
        }
        else
          exep[j] = 'X';
      }

      if (is_exep_equal_char(exep, lambda_vec,var) ) {
        if(states_visited[pp->to]) {
          pp = pp->next;
          continue;
        }
        states_to_visit.push(pp->to);
        states_visited[pp->to] = true;
        reachable.push_back(pp->to);
      }
      pp = pp->next;
    }
    kill_paths(state_paths);
  }
  delete[] indices;

  int num_initial = reachable.size();
  int num_bits = std::ceil(std::log2(num_initial));
  int len = var + num_bits;

  // one new "initial" state, which encompasses all reachable states
  // by lambda
  int num_states = dfa->ns+1;
  std::vector<std::pair<std::vector<char>,int>> state_exeps;
  indices = GetBddVariableIndices(len);
  statuses = new char[num_states+1];

  // if any of the reachable states are final, then the new
  // initial state is final
  statuses[0] = '-';
  for(int i = 0; i < reachable.size(); i++) {
    if(dfa->f[reachable[i]] == 1) {
      statuses[0] = '+';
    }
  }

  dfaSetup(num_states,len,indices);
  // setup new "initial" state first
  for(int i = 0; i < reachable.size(); i++) {
    state_paths = pp = make_paths(dfa->bddm, dfa->q[reachable[i]]);
    std::vector<char> exep(var,'X');
    while(pp) {
      if(pp->to == sink) {
        pp = pp->next;
        continue;
      }
      for(int j = 0; j < var; j++) {
        for(tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
        if(tp) {
          if(tp->value) {
            exep[j] = '1';
          } else {
            exep[j] = '0';
          }
        } else {
          exep[j] = 'X';
        }
      }

      if (!is_exep_equal_char(exep, lambda_vec,var)) {
        std::vector<char> extra_bit_value = GetBinaryFormat(i, num_bits); // i = current state
        std::vector<char> v = exep;
        v.insert(v.end(), extra_bit_value.begin(), extra_bit_value.end());
        state_exeps.push_back(std::make_pair(v, pp->to + 1));
      }
      pp = pp->next;
    }
    kill_paths(state_paths);
  }

  dfaAllocExceptions(state_exeps.size());
  for(int i = 0; i < state_exeps.size(); i++) {
    dfaStoreException(state_exeps[i].second,&state_exeps[i].first[0]);
  }
  dfaStoreState(sink+1);
  state_exeps.clear();

  // continue with rest of states
  for(int i = 0; i < dfa->ns; i++) {
    statuses[i+1] = '-';
    state_paths = pp = make_paths(dfa->bddm, dfa->q[i]);
    while(pp) {
      if (pp->to == sink) {
        pp = pp->next;
        continue;
      }
      std::vector<char> exep(var,'X');
      for (int j = 0; j < var; j++) {
        for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
        if (tp) {
          if (tp->value) exep[j] = '1';
          else exep[j] = '0';
        }
        else
          exep[j] = 'X';
      }
      // if not lambda, then add transition, with 0s padded on end
      if (!is_exep_equal_char(exep, lambda_vec,var)) {
        for (int k = 0; k < num_bits; k++) {
          exep.push_back('0');
        }
        exep.push_back('\0');
        state_exeps.push_back(std::make_pair(exep,pp->to+1));
      } else if(dfa->f[pp->to == 1]) {
        statuses[i+1] = '+';
      }
      pp = pp->next;
    }
    kill_paths(state_paths);

    dfaAllocExceptions(state_exeps.size());
    for(int j = 0; j < state_exeps.size(); j++) {
      dfaStoreException(state_exeps[j].second, &state_exeps[j].first[0]);
    }
    dfaStoreState(sink+1);
    state_exeps.clear();

    if(dfa->f[i] == 1) {
      statuses[i+1] = '+';
    }
  }

  statuses[num_states] = '\0';

  temp_dfa = dfaBuild(statuses);
  result_dfa = dfaMinimize(temp_dfa);
  dfaFree(temp_dfa);
  if(project_bit) {
    // project away the last bit as well
    num_bits++;
  }

  for(int i = 0; i < num_bits; i++) {
    int bit = len-i-1;
    temp_dfa = dfaProject(result_dfa,(unsigned)bit);
    dfaFree(result_dfa);
    result_dfa = dfaMinimize(temp_dfa);
    dfaFree(temp_dfa);
  }

  delete[] statuses;
  delete[] indices;

  return result_dfa;
}

// var should be 9
DFA_ptr RelationalStringAutomaton::trim_lambda_suffix(DFA_ptr dfa, int var, bool project_bit) {
  if(var != VAR_PER_TRACK) {
    LOG(FATAL) << "Bad nuber o bits!";
  }

  DFA_ptr result_dfa = nullptr, temp = nullptr;
  paths state_paths, pp;
  trace_descr tp;
  char* statuses = new char[dfa->ns+1];
  int *indices = Automaton::GetBddVariableIndices(var);
  int sink = find_sink(dfa);
  CHECK_GT(sink,-1);

  std::vector<std::pair<std::vector<char>,int>> state_exeps;
  std::vector<char> lambda_vec(var,'1');
  dfaSetup(dfa->ns, var, indices);
  for(int i = 0; i < dfa->ns; i++) {
    state_paths = pp = make_paths(dfa->bddm, dfa->q[i]);
    statuses[i] = '-';
    while (pp) {
      if (pp->to != sink) {
        std::vector<char> exep(var,'X');
        for (unsigned j = 0; j < var; j++) {
          for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);

          if (tp) {
            if (tp->value) exep[j] = '1';
            else exep[j] = '0';
          }
          else
            exep[j] = 'X';
        }

        bool is_lam = is_exep_equal_char(exep,lambda_vec,var);
        if (is_lam && i == pp->to) {

        }
        else {
          exep.push_back('\0');
          state_exeps.push_back(std::make_pair(exep, pp->to));
          if(is_lam && dfa->f[pp->to] == 1) {
            statuses[i] = '+';
          }
        }
      }
      pp = pp->next;
    }
    kill_paths(state_paths);

    dfaAllocExceptions(state_exeps.size());
    for (unsigned k = 0; k < state_exeps.size(); k++) {
      dfaStoreException(state_exeps[k].second,&state_exeps[k].first[0]);
    }
    dfaStoreState(sink);
    state_exeps.clear();
  }
  statuses[dfa->ns] = '\0';
  temp = dfaBuild(statuses);
  result_dfa = dfaMinimize(temp);
  dfaFree(temp);

  if(project_bit) {
    // project away extra bit
    temp = dfaProject(result_dfa, var - 1);
    dfaFree(result_dfa);
    result_dfa = dfaMinimize(temp);
    dfaFree(temp);
  }

  delete[] statuses;
  delete[] indices;

  return result_dfa;
}

DFA_ptr RelationalStringAutomaton::trim_prefix(DFA_ptr subject_dfa, DFA_ptr trim_dfa, int var) {
  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr;
  RelationalStringAutomaton_ptr temp_multi = nullptr, subject_multi = nullptr,
                          trim_multi = nullptr, intersect_multi = nullptr;
  StringAutomaton_ptr result_string_auto = nullptr;

  // (x,x,lambda) until track 2 is lambda
  // (x,lambda,x) until end
  temp_multi = makePrefixSuffix(0,1,2,3);
  subject_multi = new RelationalStringAutomaton(subject_dfa,0,3, DEFAULT_NUM_VAR);
  trim_multi = new RelationalStringAutomaton(trim_dfa,1,3, DEFAULT_NUM_VAR);

  intersect_multi = temp_multi->Intersect(subject_multi);
  delete temp_multi;
  delete subject_multi;

  temp_multi = intersect_multi;
  intersect_multi = temp_multi->Intersect(trim_multi);
  delete temp_multi;
  delete trim_multi;

  // 3rd track has lambda prefix, so get it (automatically removes lambda prefix/suffix)
  result_string_auto = intersect_multi->GetKTrack(2);
  result_dfa = dfaCopy(result_string_auto->getDFA());
  delete intersect_multi;
  delete result_string_auto;

  return result_dfa;
}

DFA_ptr RelationalStringAutomaton::trim_suffix(DFA_ptr subject_dfa, DFA_ptr trim_dfa, int var) {
  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr;
  RelationalStringAutomaton_ptr temp_multi = nullptr, subject_multi = nullptr,
                          trim_multi = nullptr, intersect_multi = nullptr;
  StringAutomaton_ptr result_string_auto = nullptr;

  // (x,x,lambda) until track 2 is lambda
  // (x,lambda,x) until end
  temp_multi = makePrefixSuffix(0,1,2,3);
  subject_multi = new RelationalStringAutomaton(subject_dfa,0,3,DEFAULT_NUM_VAR);
  // gotta prepend trim_dfa first, to go on track 3
  temp_dfa = prepend_lambda(trim_dfa,var);
  trim_multi = new RelationalStringAutomaton(temp_dfa,2,3,VAR_PER_TRACK);
  dfaFree(temp_dfa);

  intersect_multi = temp_multi->Intersect(subject_multi);
  delete temp_multi;
  delete subject_multi;

  temp_multi = intersect_multi;
  intersect_multi = temp_multi->Intersect(trim_multi);
  delete temp_multi;
  delete trim_multi;

  result_string_auto = intersect_multi->GetKTrack(1);
  result_dfa = dfaCopy(result_string_auto->getDFA());
  delete intersect_multi;
  delete result_string_auto;

  return result_dfa;
}

DFA_ptr RelationalStringAutomaton::concat(DFA_ptr prefix_dfa, DFA_ptr suffix_dfa, int var) {
  DFA_ptr temp_dfa = nullptr, result_dfa = nullptr;
  RelationalStringAutomaton_ptr temp_multi = nullptr, prefix_multi = nullptr,
                          suffix_multi = nullptr, intersect_multi = nullptr;
  StringAutomaton_ptr result_string_auto = nullptr;

  // (x,x,lambda) until track 2 is lambda
  // (x,lambda,x) until end

  temp_multi = makePrefixSuffix(0,1,2,3);
  prefix_multi = new RelationalStringAutomaton(prefix_dfa,1,3,var);
  temp_dfa = prepend_lambda(suffix_dfa,var);
  suffix_multi = new RelationalStringAutomaton(temp_dfa,2,3,VAR_PER_TRACK);
  dfaFree(temp_dfa);
  intersect_multi = temp_multi->Intersect(prefix_multi);
  delete temp_multi;
  delete prefix_multi;
  temp_multi = intersect_multi;
  intersect_multi = temp_multi->Intersect(suffix_multi);
  delete temp_multi;
  delete suffix_multi;
  result_string_auto = intersect_multi->GetKTrack(0);
  result_dfa = dfaCopy(result_string_auto->getDFA());
  delete intersect_multi;
  delete result_string_auto;
  return result_dfa;
}

DFA_ptr RelationalStringAutomaton::pre_concat_prefix(DFA_ptr concat_dfa, DFA_ptr suffix_dfa, int var) {
  return trim_suffix(concat_dfa,suffix_dfa,var);
}

DFA_ptr RelationalStringAutomaton::pre_concat_suffix(DFA_ptr concat_dfa, DFA_ptr prefix_dfa, int var) {
  return trim_prefix(concat_dfa,prefix_dfa,var);
}

void RelationalStringAutomaton::add_print_label(std::ostream& out) {
  out << " subgraph cluster_0 {\n";
  out << "  style = invis;\n  center = true;\n  margin = 0;\n";
  out << "  node[shape=plaintext];\n";
  out << " \"\"[label=\"";
  for (auto& el : formula_->get_variable_coefficient_map()) {
    out << el.first << "\n";
  }
  out << "\"]\n";
  out << " }";
}

} /* namespace Theory */
} /* namespace Vlab */