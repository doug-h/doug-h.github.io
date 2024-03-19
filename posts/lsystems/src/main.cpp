#include "app.h"
#include "lsystem.h"
#include "turtle.h"

#ifdef BUILD_WASM
#include <emscripten.h>
#endif

#include <string>

// TODO - Think about char sizes/Unicode

// Window dimensions
constexpr int WIDTH = 900;
constexpr int HEIGHT = 600;

// ImGui scroll bar that controls LSystem development
constexpr int STAGE_BAR_H = 64;
constexpr int STAGE_BAR_Y = HEIGHT - STAGE_BAR_H;

struct Demo {
  LSystem ls;
  TurtleMap tm;

  // Window params.
  SDL_FPoint origin = {WIDTH / 2.0f, STAGE_BAR_Y - 5.0f};
  float zoom = 1;
  int stage = 0;
  int max_stage = 7;
  SDL_Colour clear_colour = {0, 0, 0, 0};

  // Turtle params.
  int step_size = 5;
  float angle_delta = 0.071;
  SDL_Colour turtle_colour = {255, 0, 255, 0};
};

// The current demo, modified by UI/input functions defined below
Demo g_demo;

// Example L-Systems the user can switch between
std::vector<Demo> examples;
std::vector<const char *> example_names; // Displayed in ImGui

// Returns whether the window needs to be redrawn
bool HandleWindowEvents(SDL_Event e)
{
  switch (e.type) {

    // TODO - panning doesn't need a redraw, could draw different portion of
    // larger texture
  case SDL_MOUSEMOTION: {
    Uint32 state = SDL_GetMouseState(nullptr, nullptr);
    if (SDL_BUTTON_LMASK & state) {
      g_demo.origin.x += e.motion.xrel;
      g_demo.origin.y += e.motion.yrel;
      return true;
    }
  } break; // case SDL_MOUSEMOTION

  case SDL_MOUSEWHEEL: {
    g_demo.zoom *= (e.wheel.y > 0) ? 1.1f : 0.9f;
    return true;
  } break; // case SDL_MOUSEWHEEL
  }

  return false;
}

void ResetSystem()
{
  g_demo.ls.Reset();
  UpdateTurtleMap(g_demo.tm, g_demo.ls);
}

void Redraw()
{
  SDL_SetRenderTarget(App::renderer, App::screen);
  SDL_SetRenderDrawColor(App::renderer, g_demo.clear_colour.r,
                         g_demo.clear_colour.g, g_demo.clear_colour.b,
                         g_demo.clear_colour.a);
  SDL_RenderClear(App::renderer);

  SDL_SetRenderDrawColor(App::renderer, g_demo.turtle_colour.r,
                         g_demo.turtle_colour.g, g_demo.turtle_colour.b,
                         g_demo.turtle_colour.a);
  Draw(App::renderer, g_demo.ls.Generate(g_demo.stage), g_demo.tm,
       g_demo.origin, g_demo.step_size * g_demo.zoom, g_demo.angle_delta);
  SDL_SetRenderTarget(App::renderer, NULL);
}

void main_loop()
{
  App::ClearScreen(g_demo.clear_colour);

  ImGuiIO &io = ImGui::GetIO();

  bool system_changed = false;
  bool redraw = false;

  SDL_Event e;
  while (App::PollEvent(&e)) {
    redraw |= HandleWindowEvents(e);
  }

  // Limit text input to (char)0-127
  struct SafeChar {
    static int Filter(ImGuiInputTextCallbackData *data)
    {
      return (data->EventChar > 127);
    }
  };

  // ======= ALLOW THE USER TO EDIT THE RULES OF THE SYSTEM ==========
  //
  // Any changes will most likely require the system to be rebuilt
  //                    (system_changed = true)
  {
    ImGui::SetNextWindowSize({376, 150}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({0, 60}, ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);

    ImGui::Begin("Rules table");
    if (ImGui::InputText("seed", &(g_demo.ls.seed),
                         ImGuiInputTextFlags_CallbackCharFilter,
                         SafeChar::Filter)) {
      system_changed = true;
    }

    if (ImGui::TreeNode("randomness options...")) {
      ImGui::InputScalar("rng seed", ImGuiDataType_U32,
                         (void *)&g_demo.ls.rng_seed);
      if (ImGui::Button("New RNG seed")) {
        g_demo.ls.RegenerateRNG();
        system_changed = true;
      }
      ImGui::TreePop();
    }

    if (ImGui::BeginTable("table1", 4)) {
      for (int i = 0; i < g_demo.ls.rules.size();) {
        Rule &r = g_demo.ls.rules[i];

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(i);
        ImGui::PushItemWidth(-1);

        char target_tmp[2] = {r.target, '\0'};
        if (ImGui::InputText("target", target_tmp, 2,
                             ImGuiInputTextFlags_CallbackCharFilter,
                             SafeChar::Filter)) {
          r.target = target_tmp[0];
          system_changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID(i);
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("replacement", &r.replacement,
                             ImGuiInputTextFlags_CallbackCharFilter,
                             SafeChar::Filter)) {
          system_changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopID();

        ImGui::TableSetColumnIndex(2);
        ImGui::PushID(i);
        ImGui::PushItemWidth(-1);
        if (ImGui::InputFloat("probability", &r.probability)) {
          system_changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopID();

        ImGui::TableSetColumnIndex(3);
        ImGui::PushID(i);
        if (ImGui::Button("Remove")) {
          system_changed = true;
          // g_demo.ls.RemoveRule(i);
          g_demo.ls.rules.erase(g_demo.ls.rules.begin() + i);
        } else {
          ++i;
        }
        ImGui::PopID();
      }

      ImGui::EndTable();
      if (ImGui::Button("Add Row")) {
        g_demo.ls.rules.push_back({'\0'});
        system_changed = true;
      }
    }

    ImGui::End();
  }

  // ======= ALLOW THE USER TO EDIT THE TURTLE BEHAVIOUR  ==========
  //
  // Any changes will not change the system, but will require a redraw
  //                          (redraw = true)
  {
    ImGui::SetNextWindowSize({282, 219}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({0, 242}, ImGuiCond_Once);
    ImGui::Begin("Turtle Instructions");

    redraw |= ImGui::InputInt("step size", &(g_demo.step_size));
    redraw |= ImGui::InputFloat("angle(turns)", &(g_demo.angle_delta));

    if (ImGui::BeginTable("inst. table", 2, ImGuiTableFlags_SizingFixedFit)) {
      int id = 0;
      for (auto &[c, ins] : g_demo.tm) {
        ++id;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        char c_tmp[2] = {c, '\0'};
        ImGui::Text("%s", c_tmp);

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID(id);
        ImGui::PushItemWidth(-1);
        if (ImGui::BeginCombo("target", instuction_labels[ins])) {
          for (int n = 0; n < N_INSTRUCTIONS; n++) {
            const bool is_selected = ((TurtleInstruction)n == ins);
            if (ImGui::Selectable(instuction_labels[n], is_selected)) {
              ins = (TurtleInstruction)n;
              redraw = true;
            }
            if (is_selected) { ImGui::SetItemDefaultFocus(); }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    if (ImGui::Button("Clear unused")) { CleanTurtleMap(g_demo.tm, g_demo.ls); }
    ImGui::End();
  }

  // ======= INCREASE/DECREASE STAGE OF THE SYSTEM  ==========
  //
  // system_changed not required as lsystem will reset itself based on the stage
  //                        requested when we draw
  //                          (redraw = true)
  {
    ImGui::SetNextWindowSize({WIDTH, STAGE_BAR_H}, ImGuiCond_Always);
    ImGui::SetNextWindowPos({0, STAGE_BAR_Y}, ImGuiCond_Always);
    ImGui::Begin("Slider", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs);

    ImGui::PushItemWidth(800);
    redraw |= ImGui::SliderInt(" ", &(g_demo.stage), 0, g_demo.max_stage);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::SmallButton("+")) { ++g_demo.max_stage; }
    if (ImGui::SmallButton("-")) {
      --g_demo.max_stage;
      if (g_demo.stage > g_demo.max_stage) {
        g_demo.stage = g_demo.max_stage;
        redraw = true;
      }
    }
    ImGui::EndGroup();

    ImGui::End();
  }

  // ======= DISPLAY RAW STRING ==========
  // Useful for debugging/transferring to other programs
  {
    ImGui::SetNextWindowSize({174, 97}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({734, 361}, ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    ImGui::Begin("Raw String (preview)");
#ifdef BUILD_WASM
    // Clipboard doesn't work in the browser, so we console log instead
    if (ImGui::Button("Print to console")) {
      std::cout << g_demo.ls.m_value << '\n';
    }
#else
    if (ImGui::Button("Copy to clipboard")) {
      ImGui::LogToClipboard();
      ImGui::LogText(g_demo.ls.m_value.c_str());
      ImGui::LogFinish();
    }
#endif
    if (g_demo.ls.m_value.size() < 9000) {
      ImGui::TextWrapped("%s", g_demo.ls.m_value.c_str());
    } else {
      ImGui::TextWrapped("String has been truncated to (9000): %s",
                         g_demo.ls.m_value.substr(0, 9000).c_str());
    }
    ImGui::End();
  }

  // ======= CHANGE DRAW COLOURS ==========
  {
    ImGui::SetNextWindowSize({257, 77}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({0, 460}, ImGuiCond_Once);
    ImGui::Begin("Colour picker");

    ImVec4 tmp_clear_colour = CreateFrom(g_demo.clear_colour);
    if (ImGui::ColorEdit3("Background", (float *)&tmp_clear_colour, 0)) {
      g_demo.clear_colour = CreateFrom(tmp_clear_colour);
      redraw = true;
    }

    ImVec4 tmp_turtle_colour = CreateFrom(g_demo.turtle_colour);
    if (ImGui::ColorEdit3("Turtle", (float *)&tmp_turtle_colour, 0)) {
      g_demo.turtle_colour = CreateFrom(tmp_turtle_colour);
      redraw = true;
    }
    ImGui::End();
  }

  // ======= LOAD HARDCODED EXAMPLES (defined in examples.h) ==========
  {
    ImGui::SetNextWindowSize({123, 54}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Once);
    ImGui::Begin("Examples");
    if (ImGui::BeginCombo("Load example", nullptr, ImGuiComboFlags_NoPreview)) {
      for (int n = 0; n < examples.size(); ++n) {
        const bool is_selected = false;
        if (ImGui::Selectable(example_names[n], is_selected)) {
          g_demo = examples[n];
          system_changed = true;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::End();
  }

  // System must recalculate its value regardless of its current stage
  if (system_changed) {
    ResetSystem();
    redraw = true;
  }

  // System must redraw,
  // it may recalculate its value depending on its current stage
  if (redraw) { Redraw(); }

  App::Present();
}

int main(int argc, char *argv[])
{
  App::Setup("fern", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE,
             SDL_RENDERER_PRESENTVSYNC);

#include "examples.h"

#define ADD_EXAMPLE(INTERNAL_NAME, READABLE_NAME)                              \
  examples.push_back(INTERNAL_NAME);                                           \
  example_names.push_back(READABLE_NAME);

  ADD_EXAMPLE(AB_1_24a, "Simple branching - ABoP 1.24a");
  ADD_EXAMPLE(AB_1_24b, "Simple branching - ABoP 1.24b");
  ADD_EXAMPLE(AB_1_24c, "Simple branching - ABoP 1.24c");
  ADD_EXAMPLE(AB_1_24d, "Simple branching - ABoP 1.24d");
  ADD_EXAMPLE(AB_1_24e, "Simple branching - ABoP 1.24e");
  ADD_EXAMPLE(AB_1_24f, "Simple branching - ABoP 1.24f");

  ADD_EXAMPLE(AB_1_27, "Stochastic branching - ABoP 1.27");

  ADD_EXAMPLE(AB_1_30_a, "Acropetal development - ABoP 1.30a");
  ADD_EXAMPLE(AB_1_30_b, "Basipetal development - ABoP 1.30b");

  ADD_EXAMPLE(AB_1_31_a, "Context sensitive - ABoP 1.31a");
  ADD_EXAMPLE(AB_1_31_b, "Context sensitive - ABoP 1.31b");
  ADD_EXAMPLE(AB_1_31_c, "Context sensitive - ABoP 1.31c");
  ADD_EXAMPLE(AB_1_31_d, "Context sensitive - ABoP 1.31d");
  ADD_EXAMPLE(AB_1_31_e, "Context sensitive - ABoP 1.31e");

  ADD_EXAMPLE(GAoL_1_e, "Dragon Curve - GAoL 1e");
  ADD_EXAMPLE(GAoL_1_f, "Hilbert Curve - GAoL 1f");
  ADD_EXAMPLE(GAoL_2_a, "Sierpinski arrowhead - GAoL 2a");

  g_demo = examples[0];
  ResetSystem();
  Redraw();

#ifdef BUILD_WASM
  emscripten_set_main_loop(&main_loop, -1, 1);
#else
  while (App::Running()) {
    main_loop();
  }
#endif

  return 0;
}
