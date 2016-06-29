/*
* MultiTrackAutomaton.cpp
*
* Created on Jan 29, 2016
* Author: Will
*
*/

// TODO: getLambdaStar does not accept empty string, so it now functions like getLambdaPlus
// 			this is due to issue in string concat from libstranger


#include "MultiTrackAutomaton.h"
namespace Vlab {
namespace Theory {

MultiTrackAutomaton::TransitionTable MultiTrackAutomaton::transition_table;
const int MultiTrackAutomaton::VLOG_LEVEL = 20;

MultiTrackAutomaton::MultiTrackAutomaton(DFA_ptr dfa, int num_tracks)
			: Automaton(Automaton::Type::MULTITRACK, dfa, num_tracks * VAR_PER_TRACK),
				num_of_tracks(num_tracks), relation(nullptr) {
}

MultiTrackAutomaton::MultiTrackAutomaton(DFA_ptr dfa, int i_track, int num_tracks)
			: Automaton(Automaton::Type::MULTITRACK, nullptr, num_tracks * VAR_PER_TRACK),
			  num_of_tracks(num_tracks), relation(nullptr) {
	int *indices = getIndices(VAR_PER_TRACK);
	DFA_ptr result,temp,M;
	M = getLambdaStar(VAR_PER_TRACK,indices);
	StringAutomaton_ptr t1,t2,t3;
	t1 = new StringAutomaton(dfaCopy(dfa));
	t2 = new StringAutomaton(M);
	t3 = t1->concat(t2);
	delete t1;
	delete t2;
	M = t3->getDFA();
	trace_descr tp;
	paths state_paths, pp;
	std::vector<std::pair<std::string,int>> state_exeps;
	int sink;
	char* statuses;
	int* mindices;
	int len;
	len = num_tracks * VAR_PER_TRACK;
	mindices = getIndices(num_tracks*VAR_PER_TRACK);
	statuses = new char[len+1];
	sink = find_sink(M);
	// begin dfa building process
	dfaSetup(M->ns, len, mindices);
	for(unsigned i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);

		while(pp) {
			if(pp->to != sink) {
				std::string curr = std::string(len,'X');
				for(unsigned j = 0; j < VAR_PER_TRACK; j++) {
					for(tp = pp->trace; tp && (tp->index != mindices[j]); tp = tp->next);
					if(tp) {
						if(tp->value) curr[i_track+num_tracks*j] = '1';
						else curr[i_track+num_tracks*j] = '0';
					}
					else
						curr[i_track+num_tracks*j] = 'X';
				}
				curr.push_back('\0');
				state_exeps.push_back(std::make_pair(curr,pp->to));
			}
			pp = pp->next;
		}
		kill_paths(state_paths);

		dfaAllocExceptions(state_exeps.size());
		for(unsigned k = 0; k < state_exeps.size(); ++k) {
			std::vector<char> v = std::vector<char>(state_exeps[k].first.begin(), state_exeps[k].first.end());
			v.push_back('\0');
			dfaStoreException(state_exeps[k].second,&v[0]);
		}
		dfaStoreState(sink);
		state_exeps.clear();

		if(M->f[i] == -1)
			statuses[i] = '-';
		else if(M->f[i] == 1)
			statuses[i] = '+';
		else
			statuses[i] = '0';
	}
	statuses[len] = '\0';
	temp = dfaBuild(statuses);
	result = dfaMinimize(temp);
	dfaFree(temp);
	delete[] statuses;
	delete[] indices;
	delete[] mindices;
	delete t3;
	this->dfa = result;
}

MultiTrackAutomaton::MultiTrackAutomaton(const MultiTrackAutomaton& other)
			: Automaton(other),
				num_of_tracks(other.num_of_tracks), relation(other.relation->clone()) {
}

MultiTrackAutomaton::~MultiTrackAutomaton() {
  delete relation;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::clone() const {
	MultiTrackAutomaton_ptr cloned_auto = new MultiTrackAutomaton(*this);
	return cloned_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makePhi(int ntracks) {
	DFA_ptr non_accepting_dfa = nullptr;
	MultiTrackAutomaton_ptr non_accepting_auto = nullptr;
	int *indices = getIndices(ntracks*VAR_PER_TRACK);

	non_accepting_dfa = Automaton::makePhi(ntracks*VAR_PER_TRACK, indices);
	delete[] indices; indices = nullptr;
	non_accepting_auto = new MultiTrackAutomaton(non_accepting_dfa, ntracks);
	non_accepting_auto->setRelation(nullptr);
	return non_accepting_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeAuto(StringRelation_ptr relation) {
	MultiTrackAutomaton_ptr result_auto = nullptr;
	DVLOG(VLOG_LEVEL) << "Begin making auto....";
	switch(relation->get_type()) {
    case StringRelation::Type::EQ:
    	DVLOG(VLOG_LEVEL) << "EQ";
      result_auto = MultiTrackAutomaton::makeEquality(relation);
      break;
    case StringRelation::Type::NOTEQ:
    	DVLOG(VLOG_LEVEL) << "NOTEQ";
      result_auto = MultiTrackAutomaton::makeNotEquality(relation);
      break;
    case StringRelation::Type::GT:
    DVLOG(VLOG_LEVEL) << "GT";
      result_auto = MultiTrackAutomaton::makeGreaterThan(relation);
      break;
    case StringRelation::Type::GE:
    DVLOG(VLOG_LEVEL) << "GE";
      result_auto = MultiTrackAutomaton::makeGreaterThanOrEqual(relation);
      break;
    case StringRelation::Type::LT:
    	DVLOG(VLOG_LEVEL) << "LT";
      result_auto = MultiTrackAutomaton::makeLessThan(relation);
      break;
    case StringRelation::Type::LE:
    	DVLOG(VLOG_LEVEL) << "LE";
      result_auto = MultiTrackAutomaton::makeLessThanOrEqual(relation);
      break;
    default:
      LOG(ERROR) << "StringRelation type not supported";
      result_auto = nullptr;
      break;
	}
	DVLOG(VLOG_LEVEL) << "Done making auto.";
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeEquality(StringRelation_ptr relation) {
	MultiTrackAutomaton_ptr result_auto;
	DFA_ptr temp_dfa, result_dfa;
  std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;

	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();

  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
		StringAutomaton_ptr string_auto = StringAutomaton::makeString(left_data);
		right_track = (*trackmap)[right_data];
		result_auto = new MultiTrackAutomaton(string_auto->getDFA(),right_track,num_tracks);
		delete string_auto;
		result_auto->setRelation(relation->clone());
		return result_auto;
	} else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		StringAutomaton_ptr string_auto = StringAutomaton::makeRegexAuto(left_data);
		right_track = (*trackmap)[right_data];
		result_auto = new MultiTrackAutomaton(string_auto->getDFA(),right_track,num_tracks);
		delete string_auto;
		result_auto->setRelation(relation->clone());
		return result_auto;
  } else {
  	left_track = (*trackmap)[left_data];
	}
	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
		StringAutomaton_ptr string_auto = StringAutomaton::makeString(right_data);
		result_auto = new MultiTrackAutomaton(string_auto->getDFA(),left_track,num_tracks);
		delete string_auto;
		result_auto->setRelation(relation->clone());
		return result_auto;
	} else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		StringAutomaton_ptr string_auto = StringAutomaton::makeRegexAuto(right_data);
		result_auto = new MultiTrackAutomaton(string_auto->getDFA(),left_track,num_tracks);
		delete string_auto;
		result_auto->setRelation(relation->clone());
		return result_auto;
  } else {
  	right_track = (*trackmap)[right_data];
	}

	result_dfa = make_binary_relation_dfa(StringRelation::Type::EQ,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	result_auto->setRelation(relation->clone());
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeNotEquality(StringRelation_ptr relation) {
	MultiTrackAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr constant_string_auto = nullptr;
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
	std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;

	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();

  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
  } else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(left_data);
	} else {
  	left_track = (*trackmap)[left_data];
	}

	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(right_data);
  } else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
	} else {
  	right_track = (*trackmap)[right_data];
	}

	result_dfa = make_binary_relation_dfa(StringRelation::Type::NOTEQ,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	// if constant_string_auto != nullptr, then either the left or right
	// side of the inequality is constant; we need to intersect it with
	// the multitrack where the constant is on the extra track, then
	// project away the extra track before we return
	if(constant_string_auto != nullptr) {
	  DVLOG(VLOG_LEVEL) << "NOT EMPTY";
		MultiTrackAutomaton_ptr constant_multi_auto = new MultiTrackAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
		temp_auto = result_auto->intersect(constant_multi_auto);
		delete result_auto;
		delete constant_multi_auto;
		delete constant_string_auto;
		result_auto = temp_auto->projectKTrack(num_tracks-1);
		delete temp_auto;
	}

	result_auto->setRelation(relation->clone());
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeLessThan(StringRelation_ptr relation) {
	MultiTrackAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr constant_string_auto = nullptr;
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;


DVLOG(VLOG_LEVEL) << "TESTING REVERSE";
DVLOG(VLOG_LEVEL) << "DONE TESTING REVERSE";

	std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;
	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();
  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
  } else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(left_data);
	} else {
  	left_track = (*trackmap)[left_data];
	}
	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(right_data);
  } else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
	} else {
  	right_track = (*trackmap)[right_data];
	}
	result_dfa = make_binary_relation_dfa(StringRelation::Type::LT,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	// if constant_string_auto != nullptr, then either the left or right
	// side of the inequality is constant; we need to intersect it with
	// the multitrack where the constant is on the extra track, then
	// project away the extra track before we return
	if(constant_string_auto != nullptr) {
		DVLOG(VLOG_LEVEL) << "NOT EMPTY";
		MultiTrackAutomaton_ptr constant_multi_auto = new MultiTrackAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
		temp_auto = result_auto->intersect(constant_multi_auto);
		delete result_auto;
		delete constant_multi_auto;
		delete constant_string_auto;
		result_auto = temp_auto->projectKTrack(num_tracks-1);
		delete temp_auto;
	}

	result_auto->setRelation(relation->clone());
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeLessThanOrEqual(StringRelation_ptr relation) {
  /*
  MultiTrackAutomaton_ptr greater_than_auto = nullptr, less_than_or_equal_auto = nullptr;
	greater_than_auto = MultiTrackAutomaton::makeLessThan(relation);
	less_than_or_equal_auto = greater_than_auto->complement();
	delete greater_than_auto;
  less_than_or_equal_auto->inspectBDD();
  return less_than_or_equal_auto;
   */

  MultiTrackAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr constant_string_auto = nullptr;
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
	std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;

	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();

  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
  } else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(left_data);
	} else {
  	left_track = (*trackmap)[left_data];
	}

	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(right_data);
  } else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
	} else {
  	right_track = (*trackmap)[right_data];
	}

	result_dfa = make_binary_relation_dfa(StringRelation::Type::LE,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	// if constant_string_auto != nullptr, then either the left or right
	// side of the inequality is constant; we need to intersect it with
	// the multitrack where the constant is on the extra track, then
	// project away the extra track before we return
	if(constant_string_auto != nullptr) {
	  DVLOG(VLOG_LEVEL) << "NOT EMPTY";
		MultiTrackAutomaton_ptr constant_multi_auto = new MultiTrackAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
		temp_auto = result_auto->intersect(constant_multi_auto);
		delete result_auto;
		delete constant_multi_auto;
		delete constant_string_auto;
		result_auto = temp_auto->projectKTrack(num_tracks-1);
		delete temp_auto;
	}

	result_auto->setRelation(relation->clone());
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeGreaterThan(StringRelation_ptr relation) {
  MultiTrackAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr constant_string_auto = nullptr;
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
	std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;

	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();

  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
  } else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(left_data);
	} else {
  	left_track = (*trackmap)[left_data];
	}

	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(right_data);
  } else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
	} else {
  	right_track = (*trackmap)[right_data];
	}

	result_dfa = make_binary_relation_dfa(StringRelation::Type::GT,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	// if constant_string_auto != nullptr, then either the left or right
	// side of the inequality is constant; we need to intersect it with
	// the multitrack where the constant is on the extra track, then
	// project away the extra track before we return
	if(constant_string_auto != nullptr) {
	  DVLOG(VLOG_LEVEL) << "NOT EMPTY";
		MultiTrackAutomaton_ptr constant_multi_auto = new MultiTrackAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
		temp_auto = result_auto->intersect(constant_multi_auto);
		delete result_auto;
		delete constant_multi_auto;
		delete constant_string_auto;
		result_auto = temp_auto->projectKTrack(num_tracks-1);
		delete temp_auto;
	}

	result_auto->setRelation(relation->clone());

	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeGreaterThanOrEqual(StringRelation_ptr relation) {
  /*
  MultiTrackAutomaton_ptr less_than_auto = nullptr, greater_than_or_equal_auto = nullptr;
	less_than_auto = MultiTrackAutomaton::makeLessThan(relation);
	greater_than_or_equal_auto = less_than_auto->complement();
	delete less_than_auto;
	return greater_than_or_equal_auto;
	*/

	MultiTrackAutomaton_ptr result_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr constant_string_auto = nullptr;
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr, temp2_dfa = nullptr;
	std::map<std::string,int>* trackmap = relation->get_variable_trackmap();
	int num_tracks = trackmap->size(),
			left_track,right_track;

	StringRelation_ptr left_relation = relation->get_left(),
		                 right_relation = relation->get_right();
  std::string left_data = left_relation->get_data();
  std::string right_data = right_relation->get_data();

  if(left_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(left_data);
  } else if(left_relation->get_type() == StringRelation::Type::REGEX) {
		left_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(left_data);
	} else {
  	left_track = (*trackmap)[left_data];
	}

	if(right_relation->get_type() == StringRelation::Type::STRING_CONSTANT) {
  	right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeString(right_data);
  } else if(right_relation->get_type() == StringRelation::Type::REGEX) {
		right_track = num_tracks;
		num_tracks++;
		constant_string_auto = StringAutomaton::makeRegexAuto(right_data);
	} else {
  	right_track = (*trackmap)[right_data];
	}

	result_dfa = make_binary_relation_dfa(StringRelation::Type::GE,VAR_PER_TRACK,num_tracks,left_track,right_track);
	result_auto = new MultiTrackAutomaton(result_dfa,num_tracks);
	// if constant_string_auto != nullptr, then either the left or right
	// side of the inequality is constant; we need to intersect it with
	// the multitrack where the constant is on the extra track, then
	// project away the extra track before we return
	if(constant_string_auto != nullptr) {
	  DVLOG(VLOG_LEVEL) << "NOT EMPTY";
		MultiTrackAutomaton_ptr constant_multi_auto = new MultiTrackAutomaton(constant_string_auto->getDFA(),num_tracks-1,num_tracks);
		temp_auto = result_auto->intersect(constant_multi_auto);
		delete result_auto;
		delete constant_multi_auto;
		delete constant_string_auto;
		result_auto = temp_auto->projectKTrack(num_tracks-1);
		delete temp_auto;
	}

	result_auto->setRelation(relation->clone());
	return result_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeAnyAutoUnaligned(int num_tracks) {
	DFA_ptr result, temp;
	int len = VAR_PER_TRACK * num_tracks;
	int *mindices = Automaton::getIndices(num_tracks*VAR_PER_TRACK);
	std::vector<char> str(len, 'X');
	str.push_back('\0');

	dfaSetup(1, len, mindices);
	dfaAllocExceptions(1);
	dfaStoreException(0,&str[0]);
	dfaStoreState(0);

	temp = dfaBuild("+");
	result = dfaMinimize(temp);
	dfaFree(temp);
	delete[] mindices;
	return new MultiTrackAutomaton(result,num_tracks);
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::makeAnyAutoAligned(int num_tracks) {
	MultiTrackAutomaton_ptr aligned_auto = nullptr, any_auto = nullptr, temp_auto = nullptr;
	StringAutomaton_ptr any_string_auto = nullptr;

	aligned_auto = makeAnyAutoUnaligned(num_tracks);
	any_string_auto = StringAutomaton::makeAnyString();

	for(unsigned i = 0; i < num_tracks; i++) {
		any_auto = new MultiTrackAutomaton(any_string_auto->getDFA(),i,num_tracks);
		temp_auto = aligned_auto->intersect(any_auto);
		delete aligned_auto;
		delete any_auto;
		aligned_auto = temp_auto;
	}
	return aligned_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::complement() {
  DFA_ptr complement_dfa = nullptr;
  MultiTrackAutomaton_ptr temp_auto = nullptr, complement_auto = nullptr, aligned_universe_auto = nullptr;
	complement_dfa = dfaCopy(this->dfa);
	dfaNegation(complement_dfa);
	temp_auto = new MultiTrackAutomaton(complement_dfa,this->num_of_tracks);
	aligned_universe_auto = makeAnyAutoAligned(this->num_of_tracks);
	complement_auto = temp_auto->intersect(aligned_universe_auto);
	complement_auto->setRelation(relation->clone());
	delete temp_auto;
	delete aligned_universe_auto;

	return complement_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::union_(MultiTrackAutomaton_ptr other_auto) {
	if (this->num_of_tracks != other_auto->num_of_tracks) {
		LOG(ERROR) << "Error in MultiTrackAutomaton::union_, unequal track numbers";
		return this->clone();
	}
  DFA_ptr intersect_dfa, minimized_dfa;
	MultiTrackAutomaton_ptr union_auto;
	intersect_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaOR);
	minimized_dfa = dfaMinimize(intersect_dfa);
	dfaFree(intersect_dfa);
	union_auto = new MultiTrackAutomaton(minimized_dfa, this->num_of_tracks);
	union_auto->setRelation(relation->clone());
	return union_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::difference(MultiTrackAutomaton_ptr other_auto) {
	if (this->num_of_tracks != other_auto->num_of_tracks) {
		LOG(ERROR) << "Error in MultiTrackAutomaton::difference, unequal track numbers";
		return this->clone();
	}
	MultiTrackAutomaton_ptr complement_auto, difference_auto;
	complement_auto = other_auto->complement();
	difference_auto = this->intersect(complement_auto);
	difference_auto->setRelation(relation->clone());
	delete complement_auto; complement_auto = nullptr;
	return difference_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::intersect(MultiTrackAutomaton_ptr other_auto) {
	DFA_ptr intersect_dfa, minimized_dfa = nullptr;
	MultiTrackAutomaton_ptr intersect_auto = nullptr;
	StringRelation_ptr intersect_relation = nullptr;
	if (this->num_of_tracks != other_auto->num_of_tracks) {
		LOG(ERROR) << "Error in MultiTrackAutomaton::intersect, unequal track numbers";
		LOG(ERROR) << this->num_of_tracks << " != " << other_auto->num_of_tracks;
		return this->clone();
	}
	intersect_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaAND);
	minimized_dfa = dfaMinimize(intersect_dfa);
  dfaFree(intersect_dfa);
	intersect_auto = new MultiTrackAutomaton(minimized_dfa, this->num_of_tracks);

	if(this->relation == nullptr && other_auto->relation == nullptr) {
		LOG(FATAL) << "No relation set for either multitrack during intersection";
	} else if(other_auto->relation == nullptr) {
		intersect_relation = this->relation->clone();
	} else if(this->relation == nullptr) {
		intersect_relation = other_auto->relation->clone();
	} else {
		intersect_relation = new StringRelation();
		intersect_relation->set_type(StringRelation::Type::INTERSECT);
		intersect_relation->set_left(this->relation->clone());
		intersect_relation->set_right(other_auto->relation->clone());
		intersect_relation->set_variable_trackmap(this->relation->get_variable_trackmap());
	}
	intersect_auto->setRelation(intersect_relation);
	return intersect_auto;
}

MultiTrackAutomaton_ptr MultiTrackAutomaton::projectKTrack(int k_track) {
	MultiTrackAutomaton_ptr result_auto;
	DFA_ptr temp,result_dfa = this->dfa;
	int flag = 0;
	int *map = getIndices(this->num_of_tracks*VAR_PER_TRACK);
	for(int i = 0,k=0,l=0; i < this->num_of_variables; i++) {
	    if(i == k_track+l*this->num_of_tracks) {
	        map[i] = (this->num_of_tracks-1)*VAR_PER_TRACK+l;
	        l++;
	        continue;
	    }
	    map[i] = k++;
	}
	for(unsigned j = 0; j < VAR_PER_TRACK; j++) {
		temp = dfaProject(result_dfa,k_track+this->num_of_tracks*j);
		if(flag)
			dfaFree(result_dfa);
		result_dfa = dfaMinimize(temp);
		flag = 1;
		dfaFree(temp);
	}
	dfaReplaceIndices(result_dfa,map);
	delete[] map;
	result_auto = new MultiTrackAutomaton(result_dfa,this->num_of_tracks-1);
	if(relation != nullptr) {
		result_auto->setRelation(relation->clone());
	}
	return result_auto;
}

//TODO: Why doesn't k=0 work?
StringAutomaton_ptr MultiTrackAutomaton::getKTrack(int k_track) {
	DFA_ptr result = this->dfa, temp;
	StringAutomaton_ptr result_auto = nullptr;
	int flag = 0;

	if(k_track >= this->num_of_tracks) {
		std::cerr << "error in MultiTrackAutomaton::getKTrack" << std::endl;
		exit(1);
	} else if(this->num_of_tracks == 1) {
	    result= removeLambdaSuffix(this->dfa,VAR_PER_TRACK);
	    result_auto = new StringAutomaton(result);
		return result_auto;
	} else if(k_track == 0) {
		MultiTrackAutomaton_ptr m1, m2;
		m1 = this->clone();
		for (int i = this->num_of_tracks - 1; i > 0; i--) {
			m2 = m1->projectKTrack(i);
			delete m1;
			m1 = m2;
			m2 = nullptr;
		}
		if(find_sink(m1->dfa) != -1) {
			result = removeLambdaSuffix(m1->dfa, VAR_PER_TRACK);
			delete m1; m1 = nullptr;
			result_auto = new StringAutomaton(result);
		} else {
			delete m1; m1 = nullptr;
			return StringAutomaton::makeAnyString();
		}

		return result_auto;
	}
    // k_track needs to be mapped to indices 0-VAR_PER_TRACK
    // while all others need to be pushed back by VAR_PER_TRACK, then
    // interleaved with 1 less than current number of tracks
	int* map = getIndices(this->num_of_tracks*VAR_PER_TRACK);
    for(int i = 0; i < this->num_of_tracks; i++) {
        if(i == k_track) {
            for(int k = 0; k < VAR_PER_TRACK; k++) {
                map[i+this->num_of_tracks*k] = k;
            }
        } else {
            for(int k = 0; k < VAR_PER_TRACK; k++) {
                map[i+this->num_of_tracks*k] = VAR_PER_TRACK + i+(this->num_of_tracks-1)*k;
            }
        }
    }
	// project away all but the kth track
	for(int i = this->num_of_tracks-1; i >= 0; --i) {
		if(i != k_track) {
			for(int j = VAR_PER_TRACK-1; j>=0; --j) {
				temp = dfaProject(result,i+this->num_of_tracks*j);
				if(flag)
					dfaFree(result);
				result = dfaMinimize(temp);
				flag = 1;
				dfaFree(temp);
			}
		}
	}
	dfaReplaceIndices(result,map);
	if(find_sink(result) != -1) {
		temp = removeLambdaSuffix(result, VAR_PER_TRACK);
		dfaFree(result);
		result = temp;
		result_auto = new StringAutomaton(result);
	} else {
		dfaFree(result);
		result_auto = StringAutomaton::makeAnyString();
	}
	delete[] map;

	return result_auto;
}

std::vector<std::string> MultiTrackAutomaton::getAnAcceptingStringForEachTrack() {
  std::vector<std::string> strings(num_of_tracks, "");
  std::vector<bool>* example = getAnAcceptingWord();
  unsigned char c = 0;
  unsigned num_transitions = example->size() / num_of_variables;
  bool bit;
  unsigned sharp1 = 254, sharp2 = 255;

  for(int t = 0; t < num_transitions; t++) {
    unsigned offset = t*num_of_variables;
    for (int i = 0; i < num_of_tracks; i++) {
      for (int j = 0; j < VAR_PER_TRACK; j++) {
        bit = (*example)[offset+i+num_of_tracks*j];
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

int MultiTrackAutomaton::getNumTracks() const {
	return this->num_of_tracks;
}

StringRelation_ptr MultiTrackAutomaton::getRelation() {
  return this->relation;
}

bool MultiTrackAutomaton::setRelation(StringRelation_ptr relation) {
  if(this->relation != nullptr) {
    delete this->relation;
  }
  this->relation = relation;
  return true;
}

char* MultiTrackAutomaton::getLambda(int var) {
	return getSharp1(var); //11111111
}

// LAMBDAPLUS
DFA_ptr MultiTrackAutomaton::getLambdaStar(int var, int* indices) {
	char *lambda;
	lambda = getLambda(var);
	dfaSetup(3, var, indices);
	dfaAllocExceptions(1);
	dfaStoreException(1, lambda);
	dfaStoreState(2);
	dfaAllocExceptions(1);
	dfaStoreException(1, lambda);
	dfaStoreState(2);
	dfaAllocExceptions(0);
	dfaStoreState(2);
	delete[] lambda;

	//return dfaBuild("++-");
	return dfaBuild("-+-");
}

bool MultiTrackAutomaton::checkLambda(std::string exeps, int track_num, int num_tracks, int var) {
	for(int i = 0; i < var; i++) {
		if(exeps[track_num+num_tracks*i] != '1')
			return false;
	}
	return true;
}

DFA_ptr MultiTrackAutomaton::removeLambdaSuffix(DFA_ptr dfa, int num_vars) {
	DFA_ptr result_dfa, temp;
	paths state_paths, pp;
	trace_descr tp;
	char* statuses;
	int *indices;
	int sink;
	std::string symbol;
	std::vector<std::pair<std::string,int>> state_exeps;
	indices = Automaton::getIndices(num_vars);
	sink = find_sink(dfa);
	dfaSetup(dfa->ns, num_vars, indices);
	statuses = new char[dfa->ns+1];
	for(int i = 0; i < dfa->ns; i++) {
		if(dfa->f[i] == 1)
		  statuses[i] = '+';
		else
		  statuses[i] = '-';
		state_paths = pp = make_paths(dfa->bddm, dfa->q[i]);

		// essentially, basic idea is to keep all transitions except for lambda transitions
		// if there is a lambda transition on some state, that means that state must be
		// an accepting state. get rid of the lambda transition and make that state accepting.
		while (pp) {
			if (pp->to != sink) {
				for (unsigned j = 0; j < num_vars; j++) {
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);

					if (tp) {
						if (tp->value) symbol += '1';
						else symbol += '0';
					}
					else
						symbol += 'X';
				}
				if (MultiTrackAutomaton::checkLambda(symbol, 0, 1, num_vars)) {
					statuses[i] = '+';
				}
				else {
					state_exeps.push_back(std::make_pair(symbol, pp->to));
				}
				symbol = "";
			}
			pp = pp->next;
		}
		kill_paths(state_paths);

		dfaAllocExceptions(state_exeps.size());
		for (unsigned k = 0; k < state_exeps.size(); k++) {
			std::vector<char> v = std::vector<char>(state_exeps[k].first.begin(), state_exeps[k].first.end());
			v.push_back('\0');
			dfaStoreException(state_exeps[k].second,&v[0]);
		}
		dfaStoreState(sink);
		state_exeps.clear();
	}
	statuses[dfa->ns] = '\0';
	temp = dfaBuild(statuses);
	result_dfa = dfaMinimize(temp);
	dfaFree(temp);

	delete[] statuses;
	delete[] indices;
	return result_dfa;
}

DFA_ptr MultiTrackAutomaton::makeConcreteDFA() {
	DFA_ptr result_dfa;
	int *indices = getIndices(VAR_PER_TRACK);
	int nump = 1 << VAR_PER_TRACK;

	dfaSetup(2,VAR_PER_TRACK,indices);
	dfaAllocExceptions(nump-1);
	for(int i = 0; i < nump-1; i++) {
		std::vector<char> exep = getBinaryFormat(i,VAR_PER_TRACK);
		exep.push_back('\0');
		dfaStoreException(0,&exep[0]);
	}
	dfaStoreState(1);

	dfaAllocExceptions(0);
	dfaStoreState(1);

	result_dfa = dfaBuild("+-");
	delete[] indices;
	return result_dfa;
}

// TODO: Further optimize EQ_NO_LAMBDA/EQ_ONLY_LAMBDA
const MultiTrackAutomaton::TransitionVector& MultiTrackAutomaton::generate_transitions_for_relation(StringRelation::Type type, int bits_per_var) {
  // check table for precomputed value first
  std::pair<int,StringRelation::Type> key(bits_per_var,type);
	if(transition_table.find(key) != transition_table.end()) {
	  return transition_table[key];
	}

	// not previously computed; compute now and cache for later.
	bool final_states[6] = {false,false,false,false,false,false};
	switch(type) {
		case StringRelation::Type::EQ:
			final_states[0] = true;
			final_states[3] = true;
			break;
		case StringRelation::Type::NOTEQ:
			final_states[1] = true;
			final_states[2] = true;
			final_states[4] = true;
			final_states[5] = true;
			break;
		case StringRelation::Type::LT:
			final_states[2] = true;
			final_states[4] = true;
			break;
		case StringRelation::Type::LE:
		  final_states[0] = true;
			final_states[3] = true;
			final_states[2] = true;
			final_states[4] = true;
			break;
		case StringRelation::Type::GT:
			final_states[1] = true;
			final_states[5] = true;
			break;
		case StringRelation::Type::GE:
			final_states[0] = true;
			final_states[3] = true;
			final_states[1] = true;
			final_states[5] = true;
			break;
		case StringRelation::Type::EQ_NO_LAMBDA:
		  final_states[3] = true;
			break;
		case StringRelation::Type::EQ_ONLY_LAMBDA:
			final_states[0] = true;
			break;
		default:
			LOG(FATAL) << "Invalid relation ordering type";
			break;
	}

	std::vector<std::map<std::string,int>> states(6);
	states[0]["00"] = 3;
	states[0]["01"] = 1;
	states[0]["10"] = 2;
	states[0]["11"] = 0;

	states[1]["X1"] = 1;
	states[1]["X0"] = 4;

	states[2]["1X"] = 2;
	states[2]["0X"] = 5;

	states[3]["00"] = 3;
	states[3]["01"] = 4;
	states[3]["10"] = 5;
	states[3]["11"] = 3;

	states[4]["XX"] = 4;
	states[5]["XX"] = 5;

	TransitionVector good_trans;
	std::queue<std::pair<int,std::pair<std::string,std::string>>> next;
	next.push(std::make_pair(0,std::make_pair("","")));

	while(!next.empty()) {
		std::pair<int,std::pair<std::string,std::string>> curr = next.front();
		if(curr.second.first.size() >= bits_per_var) {
			if(final_states[curr.first]) {
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
	transition_table[key] = good_trans;

	return transition_table[key];
}

DFA_ptr MultiTrackAutomaton::make_binary_relation_dfa(StringRelation::Type type, int bits_per_var, int num_tracks, int left_track, int right_track) {
	DFA_ptr temp_dfa = nullptr, result_dfa = nullptr;
	int var = bits_per_var;
	int len = num_tracks * var;
	int *mindices = getIndices(num_tracks*var);
	int init,
	    accept = 1,
		  sink = 2;
	char statuses[4] = {"++-"};
	std::vector<std::pair<std::vector<char>,int>> exeps;
  TransitionVector tv_to_init, tv_to_accept;

  switch(type) {
		case StringRelation::Type::EQ:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ_NO_LAMBDA,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::EQ_ONLY_LAMBDA,var);
			break;
		case StringRelation::Type::NOTEQ:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ_NO_LAMBDA,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::NOTEQ,var);
			statuses[0] = '-';
			break;
		case StringRelation::Type::LT:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ_NO_LAMBDA,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::LT,var);
			statuses[0] = '-';
			break;
		case StringRelation::Type::LE:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::LT,var);
			break;
		case StringRelation::Type::GT:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ_NO_LAMBDA,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::GT,var);
			statuses[0] = '-';
			break;
		case StringRelation::Type::GE:
			tv_to_init = generate_transitions_for_relation(StringRelation::Type::EQ,var);
			tv_to_accept = generate_transitions_for_relation(StringRelation::Type::GT,var);
			break;
		default:
			DVLOG(VLOG_LEVEL) << "Invalid stringrelation type! can't make dfa...";
			return nullptr;
  }

  for(auto& t : tv_to_init) {
		std::vector<char> str(len,'X');
		for(int k = 0; k < var; k++) {
			str[left_track+num_tracks*k] = t.first[k];
			str[right_track+num_tracks*k] = t.second[k];
		}
		str.push_back('\0');
		exeps.push_back(std::make_pair(str,init));
	}

	for(auto& t : tv_to_accept) {
		std::vector<char> str(len,'X');
		for(int k = 0; k < var; k++) {
			str[left_track+num_tracks*k] = t.first[k];
			str[right_track+num_tracks*k] = t.second[k];
		}
		str.push_back('\0');
		exeps.push_back(std::make_pair(str,accept));
	}

	dfaSetup(3,len,mindices);
	dfaAllocExceptions(exeps.size());
	for(int i = 0; i < exeps.size(); i++) {
	  dfaStoreException(exeps[i].second, &(exeps[i].first)[0]);
	}
	dfaStoreState(sink);
  exeps.clear();

	/****************** accept state, everything *******************/
  dfaAllocExceptions(0);
	dfaStoreState(accept);

	/****************** sink state, nothing ************************/
	dfaAllocExceptions(0);
	dfaStoreState(sink);

	// build it!
	temp_dfa = dfaBuild(statuses);
	result_dfa = dfaMinimize(temp_dfa);
	dfaFree(temp_dfa);

	delete[] mindices;
	return result_dfa;

}
/*
MultiTrackAutomaton_ptr MultiTrackAutomaton::get_reverse_auto() {
	int initial_var_per_track = this->num_of_variables / this->num_of_tracks;
	std::map<std::pair<std::string, int>, int> same_reversed_paths;
	std::string ex;
	std::pair<std::string, int> p;

	DFA_ptr other = this->dfa;
	DFA_ptr temp,result;
	paths state_paths = nullptr, pp = nullptr;
	trace_descr tp = nullptr;
	int* indices = getIndices(this->num_of_tracks*initial_var_per_track);
	int sink = find_sink(other);

	// initialize array of reversed exceptions, for new reversed DFA
	//std::vector<std::vector<Exception>*> statepaths(other->ns+1);
	std::vector<std::vector<std::pair<std::string,int>>> state_exeps(other->ns+1);

	// for each state, grab their transitions and reverse their direction
	for(int i = 0; i < other->ns; i++) {
		state_paths = pp = make_paths(other->bddm, other->q[i]);
		while(pp) {
			// copy transition leading to final state, but with extra bits
			if(other->f[pp->to] == 1) {
				for(int j = 0; j < this->num_of_variables; j++) {
					for(tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
					if(tp) {
						if(tp->value) ex.push_back('1');
						else ex.push_back('0');
					}
					else ex.push_back('X');
				}
				ex.push_back('1');
				state_exeps[0].push_back(std::make_pair(ex,i+1));
				//p = make_pair(ex,pp->to+1);
				p = make_pair(ex,0);
				if(same_reversed_paths.count(p) == 1)
					same_reversed_paths[p]++;
				else
					same_reversed_paths[p] = 1;
			}
			ex = "";
			// continue with other transitions
			if(pp->to != sink) {
				// grab the indices/path
				for(int j = 0; j < this->num_of_variables; j++) {
					//the following for loop can be avoided if the indices are in order
					for(tp = pp->trace; tp && tp->index != indices[j]; tp = tp->next);
					if(tp) {
						if(tp->value) ex.push_back('1');
						else ex.push_back('0');
					}
					else ex.push_back('X');
				}
				ex.push_back('0');
				state_exeps[pp->to+1].push_back(std::make_pair(ex,i+1));
				p = make_pair(ex,pp->to+1);
				if(same_reversed_paths.count(p) == 1)
					same_reversed_paths[p]++;
				else
					same_reversed_paths[p] = 1;
			}
			ex = "";
			pp = pp->next;
		}
		kill_paths(state_paths);
	}

	// determine max number of bits needed
	unsigned max = 0;
	for(auto it : same_reversed_paths) {
		if(it.second > max) max = it.second;
	}
	delete[] indices;

	size_t num_variables_per_track = initial_var_per_track + 1 + std::ceil(std::log2(max));
	size_t num_variables_total_with_extrabits = num_variables_per_track * this->num_of_tracks;
	size_t number_of_extra_bits_needed = num_variables_per_track - initial_var_per_track - 1;
	size_t len = num_variables_total_with_extrabits;
	unsigned extra_bits_value = 0;
	char* statuses;

	indices = allocateMultipleAscIIIndex(this->num_of_tracks,num_variables_per_track);
	dfaSetup(other->ns+1, len, indices);
	statuses = new char[other->ns+1+1];

	unsigned* nextnum = new unsigned[other->ns+1];
	for(int i = 0; i < other->ns+1; i++) nextnum[i] = 0;

	// using the reversed transitions, make new reversed DFA
	// start with new final state

	dfaAllocExceptions(state_exeps[0].size());

	std::vector<char> binformat;

	for(int j = 0; j < state_exeps[0].size(); j++) {
		p = make_pair(state_exeps[0][j].first,0);

		if(same_reversed_paths[p] > 0) {
			binformat = Automaton::getBinaryFormat(nextnum[0],number_of_extra_bits_needed);
			nextnum[0]++;
			same_reversed_paths[p]--;
		} else {
			std::cout << "ERROR in MultiTrackAutomaton::getReverseMDFA" << std::endl;
		}

		binformat.push_back('\0');

		std::vector<char> v((state_exeps[0][j].first).begin(), (state_exeps[0][j].first).end());
		v.insert(v.end(),binformat.begin(), binformat.end());
		dfaStoreException(state_exeps[0][j].second, &v[0]);
	}
	statuses[0] = '0';
	dfaStoreState(sink+1);

	for(int i = 0; i < other->ns; i++) {
		// final states no longer accept
		if(other->f[i] == -1)
			statuses[i+1] = '-';
		else if(other->f[i] == 0 || other->f[i] == 1)
			statuses[i+1] = '0';


		dfaAllocExceptions(state_exeps[i+1].size());
		for(int j = 0; j < state_exeps[i+1].size(); j++) {

			p = make_pair(std::string(state_exeps[i+1][j].first),i+1);
			if(same_reversed_paths[p] > 0) {
				binformat = Automaton::getBinaryFormat(nextnum[i+1],number_of_extra_bits_needed);
				nextnum[i+1]++;
				same_reversed_paths[p]--;
			} else {
				std::cout << "ERROR in MultiTrackAutomaton::getReverseMDFA" << std::endl;
			}

			binformat.push_back('\0');

			std::vector<char> v((state_exeps[i+1][j].first).begin(), ((state_exeps[i+1][j].first).end());
			v.insert(v.end(),binformat.begin(), binformat.end());
			dfaStoreException(state_exeps[i+1][j].second, &v[0]);
		}
		dfaStoreState(sink+1);
	}

	// previous initial state is now the final state
	statuses[1] = '+';
	if(this->dfa->f[0] == 1) statuses[0] = '+';
	statuses[other->ns+1] = '\0';
	temp = dfaBuild(statuses);
	delete[] statuses;

	// project last bits away
	for(int i = 0; i < number_of_extra_bits_needed +1; i++) {
		result = dfaProject(temp,num_variables_per_track-(i+1));
		dfaFree(temp);
		temp = dfaMinimize(result);
		dfaFree(result);
	}
	return new MultiTrackAutomaton(temp,this->num_of_tracks);
}
*/
} /* namespace Vlab */
} /* namespace Theory */
