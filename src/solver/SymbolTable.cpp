/*
 * SymbolTable.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: baki
 */

#include "SymbolTable.h"

namespace Vlab {
namespace Solver {

using namespace SMT;

const int SymbolTable::VLOG_LEVEL = 10;
const char SymbolTable::ARITHMETIC[] = "__VLAB_CS_ARITHMETIC__";

SymbolTable::SymbolTable()
        : global_assertion_result(true), bound(50) {
  addVariable(new Variable(ARITHMETIC, Variable::Type::INT));
}

SymbolTable::~SymbolTable() {
  for (auto& map_pair : variable_value_table) {
    for (auto& value_pair : map_pair.second) {
      delete value_pair.second;
    }
  }
  variable_value_table.clear();

  for (auto& entry : variables) {
    delete entry.second;
  }
}

bool SymbolTable::isSatisfiable() {
  return global_assertion_result;
}

void SymbolTable::updateSatisfiability(bool value) {
  global_assertion_result = global_assertion_result and value;
}

void SymbolTable::setScopeSatisfiability(bool value) {
  is_scope_satisfiable[scope_stack.back()] = value;
}

/**
 * Unions values of variables if there is disjunction
 * (May need a fix when doing union for binary int automaton)
 * TODO Test with disjunctions
 */
void SymbolTable::unionValuesOfVariables(Script_ptr script) {
  if (scopes.size() < 2) {
    return;
  } else if (variable_value_table[script].size() > 0) { // a union operation is done before
    return;
  }

  push_scope(script);
  for (auto variable_entry : variables) {
    Value_ptr value = nullptr;
    for (auto scope : scopes) {

      // first check and merge substitution rules
      auto substituted_variable = get_substituted_variable(scope, variable_entry.second);
      if (substituted_variable != nullptr) { // update rule in the global scope
        merge_variable_substitution_rule_into_current_scope(scope, variable_entry.second);
      } else {
        substituted_variable = variable_entry.second;
      }

      // union values
      if (is_scope_satisfiable[scope]) {
        auto scope_var_value = variable_value_table[scope].find(substituted_variable);
        if (scope_var_value != variable_value_table[scope].end()) {
          if (value) {
            Value_ptr tmp = value;
            value = tmp->union_(scope_var_value->second);
            delete tmp;
          } else {
            value = scope_var_value->second->clone();
          }
        }
      }

    }
    if (value) {
      setValue(variable_entry.second, value);
    }
  }
  pop_scope();
}

/**
 * Removes let scope and all its data
 */
void SymbolTable::clearLetScopes() {
  for (auto sit = scopes.begin(); sit != scopes.end(); ) {
    if (dynamic_cast<Let_ptr>(*sit) not_eq nullptr) {
      for (auto it = variable_value_table[*sit].begin(); it != variable_value_table[*sit].end(); ) {
        delete it->second; it->second = nullptr;
        it = variable_value_table[*sit].erase(it);
      }
      variable_value_table.erase(*sit);
      sit = scopes.erase(sit);
    } else {
      sit++;
    }
  }
}

void SymbolTable::addVariable(Variable_ptr variable) {
  auto result = variables.insert(std::make_pair(variable->getName(), variable));
  if (not result.second) {
    LOG(FATAL)<< "Duplicate variable definition: " << *variable;
  }
}

Variable_ptr SymbolTable::getVariable(std::string name) {
  auto it = variables.find(name);
  CHECK(it != variables.end()) << "Variable is not found: " << name;
  return it->second;
}

Variable_ptr SymbolTable::getVariable(Term_ptr term_ptr) {
  if (QualIdentifier_ptr variable_identifier = dynamic_cast<QualIdentifier_ptr>(term_ptr)) {
    return getVariable(variable_identifier->getVarName());
  }
  return nullptr;
}

VariableMap& SymbolTable::getVariables() {
  return variables;
}

void SymbolTable::add_component(Component_ptr component){
  components_[scope_stack.back()].push_back(component);
}

void SymbolTable::add_components(std::vector<Component_ptr>& components) {
  components_[scope_stack.back()].insert(components_[scope_stack.back()].end(), components.begin(), components.end());
}

std::vector<Component*> SymbolTable::get_components_at(Visitable_ptr scope){
  return components_[scope];
}

ComponentMap SymbolTable::get_component_map(){
  return components_;
}

int SymbolTable::get_number_of_components(Visitable_ptr scope){
  return components_[scope].size();
}

/*int SymbolTable::getReuse(){
  return reuse; 
}

void SymbolTable::incrementReuse(){
  reuse++; 
}*/


Variable_ptr SymbolTable::getSymbolicVariable() {
  auto it = std::find_if(variables.begin(), variables.end(),
      [](std::pair<std::string, Variable_ptr> entry) -> bool {
        return entry.second->isSymbolic();
  });
  if (it != variables.end()) {
    return it->second;
  }
  LOG(FATAL) << "no symbolic variable found";
  return nullptr;
}

int SymbolTable::get_num_of_variables(Variable::Type type) {
  int count = 0;
  for (auto entry : variables) {
    if (type == entry.second->getType()) {
      ++count;
    }
  }

  if (Variable::Type::INT == type) {
    --count; // exclude artificial int variable
  }

  return count;
}

void SymbolTable::setBound(int bound) {
  this->bound = bound;
}

int SymbolTable::getBound() {
  return bound;
}

void SymbolTable::push_scope(Visitable_ptr key) {
  scope_stack.push_back(key);
  scopes.insert(key);
}

Visitable_ptr SymbolTable::top_scope() {
  return scope_stack.back();
}

void SymbolTable::pop_scope() {
  scope_stack.pop_back();
}


void SymbolTable::increment_count(Variable_ptr variable) {
  variable_counts_table[scope_stack.back()][variable]++;
}

void SymbolTable::decrement_count(Variable_ptr variable) {
  variable_counts_table[scope_stack.back()][variable]--;
}

int SymbolTable::get_local_count(Variable_ptr variable) {
  return variable_counts_table[scope_stack.back()][variable];
}

int SymbolTable::get_total_count(Variable_ptr variable) {
  int total = 0;
  for (auto& counters : variable_counts_table) {
    total += counters.second[variable];
  }
  return total;
}
void SymbolTable::reset_count() {
  variable_counts_table.clear();
}

bool SymbolTable::add_variable_substitution_rule(Variable_ptr subject_variable, Variable_ptr target_variable) {
  auto result = variable_substitution_table[scope_stack.back()].insert(std::make_pair(subject_variable, target_variable));
  return result.second;
}

bool SymbolTable::remove_variable_substitution_rule(SMT::Variable_ptr variable) {
  auto it = variable_substitution_table[scope_stack.back()].find(variable);
  if (it != variable_substitution_table[scope_stack.back()].end()) {
    variable_substitution_table[scope_stack.back()].erase(it);
    return true;
  }
  return false;
}

bool SymbolTable::is_variable_substituted(Visitable_ptr scope, Variable_ptr variable) {
  auto it = variable_substitution_table[scope].find(variable);
  if (it != variable_substitution_table[scope].end()) {
    return true;
  }
  return false;
}

bool SymbolTable::is_variable_substituted(Variable_ptr variable) {
  return is_variable_substituted(scope_stack.back(), variable);
}

Variable_ptr SymbolTable::get_substituted_variable(Visitable_ptr scope, Variable_ptr variable) {
  auto it = variable_substitution_table[scope].find(variable);
  if (it != variable_substitution_table[scope].end()) {
    return it->second;
  }
  return nullptr;
}

Variable_ptr SymbolTable::get_substituted_variable(Variable_ptr variable) {
  return get_substituted_variable(scope_stack.back(), variable);
}

int SymbolTable::get_num_of_substituted_variables(Visitable_ptr scope, Variable::Type type) {
  int count = 0;
  for (auto& rule : variable_substitution_table[scope]) {
      if (rule.first->getType() == type and rule.second->getType() == type) {
        ++count;
      }
  }
  return count;
}

void SymbolTable::merge_variable_substitution_rule_into_current_scope(Visitable_ptr scope, Variable_ptr variable) {
  auto substituted_variable = get_substituted_variable(scope, variable);
  if (substituted_variable != nullptr) { // update rule in the global scope
    auto current_scope_substitution = get_substituted_variable(substituted_variable);
    if (current_scope_substitution != nullptr) { // if there is a reverse rule already in global scope, remove rule, do not add any rule
      remove_variable_substitution_rule(substituted_variable);
    } else {
      add_variable_substitution_rule(variable, substituted_variable); // adds rule to global scope
    }
  }
}

Value_ptr SymbolTable::getValue(std::string var_name) {
  return getValue(getVariable(var_name));
}

Value_ptr SymbolTable::getValue(Variable_ptr variable) {
  Value_ptr result = nullptr;

  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); it++) {
    auto entry = variable_value_table[(*it)].find(variable);
    if (entry != variable_value_table[(*it)].end()) {
      return entry->second;
    }
  }

  switch (variable->getType()) {
  case Variable::Type::BOOL:
    LOG(FATAL) << "bool variables are not supported explicitly yet: " << *variable;
    break;
  case Variable::Type::INT:
    result = new Value(Theory::IntAutomaton::makeAnyInt());
    DVLOG(VLOG_LEVEL) << "initialized variable as any integer: " << *variable;
    break;
  case Variable::Type::STRING:
    result = new Value(Theory::StringAutomaton::makeAnyString());
    DVLOG(VLOG_LEVEL) << "initialized variable as any string: " << *variable;
    break;
  default:
    LOG(FATAL) << "unknown variable type" << *variable;
    break;
  }
  setValue(variable, result);
  return result;
}

VariableValueMap& SymbolTable::getValuesAtScope(Visitable_ptr scope) {
  return variable_value_table[scope];
}

bool SymbolTable::setValue(std::string var_name, Value_ptr value) {
return setValue(getVariable(var_name), value);
}

bool SymbolTable::setValue(Variable_ptr variable, Value_ptr value) {
  variable_value_table[scope_stack.back()][variable] = value;
  return true;
}

/**
 * Intersect old value of the variable with new value and sets the
 * intersection as newest value.
 */
bool SymbolTable::updateValue(std::string var_name, Value_ptr value) {
  return updateValue(getVariable(var_name), value);
}

/**
 * Intersect old value of the variable with new value and sets the
 * intersection as newest value.
 * Deletes old value if it is read from same scope
 */
bool SymbolTable::updateValue(Variable_ptr variable, Value_ptr value) {
  Value_ptr variable_old_value = getValue(variable);
  Value_ptr variable_new_value = variable_old_value->intersect(value);

  if (variable_value_table[scope_stack.back()][variable] == variable_old_value) {
    setValue(variable, variable_new_value);
    delete variable_old_value; variable_old_value = nullptr;
  } else {
    setValue(variable, variable_new_value);
  }

  return true;
}

} /* namespace Solver */
} /* namespace Vlab */

