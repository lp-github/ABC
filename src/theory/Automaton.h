/*
 * Automaton.h
 *
 *  Created on: Jun 24, 2015
 *      Author: baki
 */

#ifndef THEORY_AUTOMATON_H_
#define THEORY_AUTOMATON_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glog/logging.h>
#include <mona/bdd.h>
#include <mona/bdd_external.h>
#include <mona/dfa.h>
#include <mona/mem.h>

#include "../utils/Cmd.h"
#include "../utils/Math.h"
#include "../boost/multiprecision/cpp_int.hpp"
#include "../Eigen/SparseCore"
#include "Graph.h"
#include "GraphNode.h"
#include "options/Theory.h"
#include "SymbolicCounter.h"

namespace Vlab {
namespace Theory {

class Automaton;
using Automaton_ptr = Automaton*;
using DFA_ptr = DFA*;

using Node = std::pair<int ,int>; // pair.first = node id, pair.second node data
using BigInteger = boost::multiprecision::cpp_int;
using NextState = std::pair<int, std::vector<bool>>;

// for toDotAscii from libstranger
typedef struct CharPair_ {
	unsigned char first;
	unsigned char last;
} CharPair, *pCharPair;

class Automaton {
public:
  enum class Type
    : int {
      NONE = 0, BOOL, UNARY, INT, BINARYINT, STRING, MULTITRACK
  };

  Automaton(Automaton::Type type);
  Automaton(Automaton::Type type, DFA_ptr dfa, int num_of_variables);
  Automaton(const Automaton&);
  virtual ~Automaton();
  virtual Automaton_ptr clone() const = 0;

  virtual std::string str() const;
  virtual Automaton::Type getType() const;
  unsigned long getId();

  DFA_ptr getDFA();
  int get_number_of_bdd_variables();

  class Name {
  public:
    static const std::string NONE;
    static const std::string BOOL;
    static const std::string UNARY;
    static const std::string INT;
    static const std::string STRING;
    static const std::string BINARYINT;
  };

  bool IsEqual(Automaton_ptr other_auto);
  bool is_empty_language();
  bool is_initial_state_accepting();
  bool isOnlyInitialStateAccepting();
  bool isCyclic();
  bool isInCycle(int state);
  bool isStateReachableFrom(int search_state, int from_state);
  BigInteger Count(const unsigned long bound);
  BigInteger CountByMatrixMultiplication(const unsigned long bound);
  virtual BigInteger SymbolicCount(int bound, bool count_less_than_or_equal_to_bound = true);
  virtual BigInteger SymbolicCount(double bound, bool count_less_than_or_equal_to_bound = true);
  SymbolicCounter GetSymbolicCounter();

  Graph_ptr toGraph();

  void toDotAscii(bool print_sink = false, std::ostream& out = std::cout);
  // TODO merge toDot methods into one with options
  void ToDot(std::ostream& out = std::cout, bool print_sink = false);
  void toBDD(std::ostream& out = std::cout);
  void exportDfa(std::string file_name);
  DFA_ptr importDFA(std::string file_name);
  int inspectAuto(bool print_sink = false, bool force_mona_format = false);
  int inspectBDD();

  int GetSinkState();

  friend std::ostream& operator<<(std::ostream& os, const Automaton& automaton);

protected:
  /**
   * Generates a dfa that accepts nothing
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakePhi(const int number_of_bdd_variables);

  /**
   * Generates a dfa that accepts any input
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakeAny(const int number_of_bdd_variables);

  /**
   * Generates a dfa that accepts any input except one
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakeAnyButNotEmpty(const int number_of_bdd_variables);

  /**
   * Generates a dfa that has an accepting initial state without any loop
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakeEmpty(const int number_of_bdd_variables);

  static DFA_ptr DFAIntersect(DFA_ptr dfa1, DFA_ptr dfa2);
  static DFA_ptr DFAUnion(DFA_ptr dfa1, DFA_ptr dfa2);
  static DFA_ptr DFAProjectAway(int index, DFA_ptr dfa);
  static DFA_ptr DFAProjectAwayAndReMap(int index, int number_of_bdd_variables, DFA_ptr dfa);
//  static DFA_ptr DFAProjectAway(std::vector<int> index, int num_of_variables, DFA_ptr dfa);
  static DFA_ptr DFAProjectTo(int index, int num_of_variables, DFA_ptr dfa);

  /**
   * Generates a dfa that accepts any input that has length between start and end inclusive
   * @param start
   * @param end
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakeAcceptingAnyWithInRange(const int start, const int end, const int number_of_bdd_variables);

  /**
   * Generates a dfa that accepts any input after reading the given number of inputs
   * @param start
   * @param number_of_bdd_variables
   * @return
   */
  static DFA_ptr DFAMakeAcceptingAnyAfterLength(const int length, const int number_of_bdd_variables);

  static std::set<std::string> DFAGetTransitionsFromTo(DFA_ptr dfa, const int from, const int to, const int num_of_variables);

  bool isAcceptingSingleWord();
  // TODO update it to work for non-accepting inputs
  std::vector<bool>* getAnAcceptingWord(std::function<bool(unsigned& index)> next_node_heuristic = nullptr);
  std::vector<char> decodeException(std::vector<char>& exception);
  virtual void add_print_label(std::ostream& out);

  static int* GetBddVariableIndices(const int number_of_bdd_variables);
  static int* CreateBddVariableIndices(const int number_of_bdd_variables);

  // TODO remove vector<char> version of binary format and use string version below
  static std::vector<char> GetBinaryFormat(unsigned long n, int bit_length);
  static std::vector<char> GetReversedBinaryFormat(unsigned long n, int bit_length);

  static std::string GetBinaryStringMSB(unsigned long n, int bit_length);

  // TODO return string instead of vector<char>
  static std::vector<char> getReservedWord(char last_char, int length, bool extra_bit = false);
  void Minimize();
  void ProjectAway(unsigned index);

  bool is_start_state(int state_id);
  bool is_sink_state(int state_id);
  bool is_accepting_state(int state_id);

  bool hasIncomingTransition(int state);
  bool isStartStateReachableFromAnAcceptingState();
  bool hasNextState(int state, int search);
  int getNextState(int state, std::vector<char>& exception);
  std::set<int> getNextStates(int state);
  std::vector<NextState> getNextStatesOrdered(int state, std::function<bool(unsigned& index)> next_node_heuristic = nullptr);
  std::set<int> getStatesReachableBy(int walk);
  std::set<int> getStatesReachableBy(int min_walk, int max_walk);
  bool getAnAcceptingWord(NextState& state, std::map<int, bool>& is_stack_member, std::vector<bool>& path, std::function<bool(unsigned& index)> next_node_heuristic = nullptr);

  virtual void SetSymbolicCounter();
  virtual void decide_counting_schema(Eigen::SparseMatrix<BigInteger>& mm);
  void generateGFScript(int bound, std::ostream& out = std::cout, bool count_less_than_or_equal_to_bound = true);
  void generateMatrixScript(int bound, std::ostream& out = std::cout, bool count_less_than_or_equal_to_bound = true);


  bool isCyclic(int state, std::map<int, bool>& is_discovered, std::map<int, bool>& is_stack_member);
  bool isStateReachableFrom(int search_state, int from_state, std::map<int, bool>& is_stack_member);

  /*
   * Operations from LIBSTRANGER
   */
  void getTransitionCharsHelper(pCharPair result[], char* transitions, int* indexInResult, int currentBit, int var);
  void getTransitionChars(char* transitions, int var, pCharPair result[], int* pSize);
  char** mergeCharRanges(pCharPair charRanges[], int* p_size);
  void charToAsciiDigits(unsigned char ci, char s[]);
  void charToAscii(char* asciiVal, unsigned char c);
  void fillOutCharRange(char* range, char firstChar, char lastChar);
  char* bintostr(unsigned long, int k);
  unsigned char strtobin(char* binChar, int var);
  static int find_sink(DFA_ptr dfa);


  static unsigned long next_id;

  /**
   * Bdd variable indices cache used in MONA dfa manipulation
   */
  static std::unordered_map<int, int*> bdd_variable_indices;

  /**
   * Automaton id used for debuggin purposes
   */
  unsigned long id_;

  const Automaton::Type type_; // TODO remove type


  bool is_counter_cached_;

  /**
   * Number of bdd variables used in MONA representation
   */
  int num_of_bdd_variables_;

  /**
   * Mona dfa pointer
   */
  DFA_ptr dfa_;

  /**
   * Model counter function
   */
  SymbolicCounter counter_;
private:
  char* getAnExample(bool accepting=true); // MONA version
  // for debugging
  static int name_counter;
  static const int VLOG_LEVEL;
};

} /* namespace Theory */
} /* namespace Vlab */

#endif /* THEORY_AUTOMATON_H_ */
