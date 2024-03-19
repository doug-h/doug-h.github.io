#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Special chars for context rules
// TODO - check if this is a bad idea...
const char CON_END = '\0'; // Passed when the context is empty (no more chars)
const char CON_IGNORE = '\1';   // Rule doesn't care about context
const char CON_WILDCARD = '\2'; // Any non-null char will match

struct Rule {
  bool Match(char t, char c_l, char c_r) const {
    bool l_matches = (left_context == c_l) or (left_context == CON_IGNORE) or
                     (left_context == CON_WILDCARD and c_l != CON_END);
    bool r_matches = (right_context == c_r) or (right_context == CON_IGNORE) or
                     (right_context == CON_WILDCARD and c_r != CON_END);
    return (t == target) and l_matches and r_matches;
  }

  char target;
  std::string replacement = "";

  char left_context = CON_IGNORE;
  char right_context = CON_IGNORE;
  float probability = 1.0f;
};

struct LSystem {
  void Reset();

  bool CharUsed(char c) const;

  const std::string &Generate(int stage);
  void Step();
  void RegenerateRNG();

  std::string FindReplacement(char t, char c_l, char c_rs) const;
  char FindLNeighbour(int position, int depth = 0) const;
  char FindRNeighbour(int position, int depth = 0) const;

  std::string seed;
  std::vector<Rule> rules;
  std::string m_value;
  int m_stage;

  void AddIgnored(char c) {
    unsigned char index = c;
    assert(index < 128);
    ignore_list[(index > 63)] |= (1ull << (index % 64));
  }

  bool IsIgnored(char c) const {
    unsigned char index = c;
    assert(index < 128);
    return ignore_list[(index > 63)] & (1ull << (index % 64));
  }

  // When context matching, we ignore these chars
  // ImGui restricts char input to 0-127
  uint64_t ignore_list[2] = {0};

  uint32_t rng_seed = time(NULL);
};

// Returns the value of the system at the provided stage. This may involve
// resetting and/or advancing the system depending on it's current state.
const std::string &LSystem::Generate(int stage) {
  if (m_stage > stage) {
    Reset();
  }

  for (int i = m_stage; i < stage; ++i) {
    Step();
  }

  return m_value;
}

void LSystem::Reset() {
  m_stage = 0;
  m_value = seed;
}

void LSystem::RegenerateRNG() { rng_seed = rand(); }

// Does this char appear anywhere in the system
bool LSystem::CharUsed(char c) const {
  if (seed.find(c) != std::string::npos) {
    // Used in seed
    return true;
  }
  for (Rule r : rules) {
    if (r.target == c or r.left_context == c or r.right_context == c) {
      return true;
    }
    if (r.replacement.find(c) != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::string LSystem::FindReplacement(char t, char c_l, char c_r) const {
  // Since rules are currently in a flat list, we just try each one until one
  // works. For stochastic rules we need to keep track of the probability.

  // Only one probability is needed, as although the list of rules is not sorted
  // only one target can match the input at a time.

  // TODO - Context sensitive rules should take priority
  float s = rand() / (float)RAND_MAX;
  for (const Rule &r : rules) {
    if (r.Match(t, c_l, c_r)) {
      if (s < r.probability) {
        return r.replacement;
      } else {
        s -= r.probability;
      }
    }
  }

  return std::string(1, t);
}
void LSystem::Step() {
  ++m_stage;

  // Seed the rng so the output is constant
  srand(rng_seed);

  std::string next = "";

  for (int idx_c = 0; idx_c < m_value.size(); ++idx_c) {

    char c = m_value[idx_c];
    char l_context = FindLNeighbour(idx_c - 1);
    char r_context = FindRNeighbour(idx_c + 1);

    next += FindReplacement(c, l_context, r_context);
  }

  m_value = next;
}

char LSystem::FindLNeighbour(int position, int depth) const {
  /* Description taken from 'A MODEL STUDY ON BIOMORPHOLOGICAL DESCRIPTION'. P. HOGEWEG (1973)
(1) When the left neighbouring symbol is an
  alphabetic symbol, this symbol defines the context (as
  is the case in non-bracketed iL systems).
(2) When the left neighbouring symbol is an open-
  ing bracket, the leftsided context is defined by the first
  alphabetic symbol in the string towards the left, which
  is separated from this opening bracket by an equal
  number (possibly 0) of opening and closing brackets.
(3) When the left neighbouring symbol is a closing
  bracket, the leftsided context is defined by the first al-
  phabetic symbol towards the left which is separated
  from the symbol by an equal number of opening and
  closing brackets (including the neighbouring closing
  bracket).

NOTE - only production rules of the form X -> A[B] where A contains some
non-ignored symbols will work correctly (otherwise multiple left-brackets will
confuse the code)
*/

  while (position >= 0) {

    char v = m_value[position];
    if (IsIgnored(v)) {
      --position;
      continue;
    }
    if (v == '[') {
      --position;
      --depth;
      depth = std::max(depth, 0);
    } else if (v == ']') {
      --position;
      ++depth;
    } else {
      if (depth == 0) {
        return v;
      } else {
        --position;
      }
    }
  }
  return CON_END;
}

char LSystem::FindRNeighbour(int position, int depth) const {
  /*Description taken from 'A MODEL STUDY ON BIOMORPHOLOGICAL DESCRIPTION'. P. HOGEWEG (1973)
  (1) When the right neighbouring symbol is an
  alphabetic symbol, this symbol defines the context (as
  is the case in non-bracketed iL systems).
  (4) When the right neighbouring symbol is a closing
  bracket, the rightsided context is defined by the symbol
  at the end of the string, which represents the environ-
  ment.
  (5) When the right-sided symbol is an opening
  bracket, the rightsided context is defined bv the first al-
  phabetic symbol towards the right, which is separated
  from the symbol by an equal number of opening and
  closing brackets (including the neighbouring closing
  bracket). !!! This may be a mistake in the paper !!!
  */
  while (position < m_value.size()) {

    char v = m_value[position];
    if (IsIgnored(v)) {
      ++position;
    } else if (v == '[') {
      ++position;
      ++depth;
    } else if (v == ']') {
      if (depth == 0) {
        return CON_END;
      } else {
        ++position;
        --depth;
      }
    } else { // v is valid symbol
      if (depth == 0) {
        return v;
      } else {
        ++position;
      }
    }
  }
  return CON_END;
}
