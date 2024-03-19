#pragma once

#include "lsystem.h"

#include "SDL.h"

#include <map>

/*
 * test
 */

#define AS_ENUM(a) INS_##a,
#define AS_ARRAY(a) #a,

#define INSTRUCTIONS(X)                                                        \
  X(NONE)                                                                      \
  X(MOVE_FORWARD)                                                              \
  X(TURN_LEFT)                                                                 \
  X(TURN_RIGHT)                                                                \
  X(PUSH_POSITION)                                                             \
  X(POP_POSITION)                                                              \
  X(DRAW_SQUARE)

enum TurtleInstruction {
  // populated by X-Macro defined above
  INSTRUCTIONS(AS_ENUM)

      N_INSTRUCTIONS,
};
using TurtleMap = std::map<char, TurtleInstruction>;

const char *instuction_labels[N_INSTRUCTIONS] = {
    INSTRUCTIONS(AS_ARRAY)}; // To display in UI

// Add any new symbols from the LSystem to the map
void UpdateTurtleMap(TurtleMap &tm, const LSystem &ls) {
  for (Rule r : ls.rules) {
    const char t = r.target;
    if (t != '\0' and tm.count(t) == 0) {
      tm[t] = INS_NONE;
    }
    for (auto c : r.replacement) {
      if (c != '\0' and tm.count(c) == 0) {
        tm[c] = INS_NONE;
      }
    }
  }
  for (char c : ls.seed) {
    if (c != '\0' and tm.count(c) == 0) {
      tm[c] = INS_NONE;
    }
  }
}

// Remove any symbols which no longer appear in the LSystem
void CleanTurtleMap(TurtleMap &tm, const LSystem &ls) {
  for (auto it = tm.begin(); it != tm.end();) {
    auto [c, ins] = *it;
    if (!ls.CharUsed(c)) {
      it = tm.erase(it);
    } else {
      ++it;
    }
  }
}

// For each symbol in the provided string, the turtle checks if there is an
// associated instruction in TurtleMap and if so, does it.
void Draw(SDL_Renderer *r, const std::string &instructions, const TurtleMap &tm,
          SDL_FPoint origin, float step, float da) {

  // This number was not chosen for any reason, but generating
  // arbitrarily large vectors based on a user typo would be a bad idea...
  const int MAX_STACK_SIZE = 1 << 16;

  const float TWO_PI = 6.283185307;

  struct State {
    float x, y, a;
  } state = {origin.x, origin.y, 0.75f};

  std::vector<State> stack;

  for (char c : instructions) {
    auto it = tm.find(c);
    if (it == tm.end()) {
      continue;
    }
    TurtleInstruction ti = it->second;
    switch (ti) {
    case INS_MOVE_FORWARD: {
      SDL_FPoint dp = {-step * cosf(TWO_PI * state.a),
                       step * sinf(TWO_PI * state.a)};
      SDL_RenderDrawLineF(r, state.x, state.y, state.x + dp.x, state.y + dp.y);
      state.x += dp.x;
      state.y += dp.y;
    } break;
    case INS_TURN_LEFT: {
      state.a += da;
    } break;
    case INS_TURN_RIGHT: {
      state.a -= da;
    } break;
    case INS_PUSH_POSITION: {
      if (stack.size() < MAX_STACK_SIZE) {
        stack.push_back(state);
      }
    } break;
    case INS_POP_POSITION: {
      if (!stack.empty()) {
        state = stack.back();
        stack.pop_back();
      }
    } break;
    case INS_DRAW_SQUARE: {
      const float sq_w = 0.25f * step;
      SDL_FRect _r{state.x - sq_w / 2, state.y - sq_w / 2, sq_w, sq_w};
      SDL_RenderFillRectF(r, &_r);
    } break;
    case INS_NONE: {
    } break;
    }
  }
}
