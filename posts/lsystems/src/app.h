#pragma once
#include "SDL.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

#include <algorithm>
#include <string>

/*
There is only ever one SDL_Window, SDL_Renderer and ImGuiContext, that all
persist for the lifetime of the program.
Just call App::Setup somewhere near the start of main.
*/

namespace ImGui {
// defined in /imgui/misc/cpp/imgui_stdlib.cpp
IMGUI_API bool InputText(const char *label, std::string *buf,
                         ImGuiInputTextFlags flags = 0,
                         ImGuiInputTextCallback callback = NULL,
                         void *user_data = NULL);
} // namespace ImGui

ImVec4 CreateFrom(SDL_Colour sc) {
  // [0,255] -> [0,1] with float weirdness
  return {sc.r / 255.0f, sc.g / 255.0f, sc.b / 255.0f, sc.a / 255.0f};
}
Uint8 AsPixel(float p) {
  // A bit suss, but mirrors what ImGui does
  return ((int)(std::clamp(p, 0.0f, 1.0f) * 255.0f + 0.5f));
}
SDL_Colour CreateFrom(ImVec4 iv) {
  return {AsPixel(iv.x), AsPixel(iv.y), AsPixel(iv.z), AsPixel(iv.w)};
}

namespace App {
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Texture *screen = nullptr;

bool g_alive;

// Call all the SDL/imgui setup functions
void Setup(const char *name, int width, int height, SDL_WindowFlags,
           SDL_RendererFlags);

bool Running() { return g_alive; }
bool PollEvent(SDL_Event *e);

// The ClearScreen / Present pair should be called every frame so ImGui is
// responsive
void ClearScreen(SDL_Colour);
void Present();

void Quit() { g_alive = false; }
}; // namespace App

void App::Setup(const char *name, int width, int height,
                SDL_WindowFlags w_flags, SDL_RendererFlags r_flags) {

  w_flags = SDL_WindowFlags(
      w_flags & (~SDL_WINDOW_RESIZABLE)); // Resizing not currently supported

  App::window = SDL_CreateWindow(name, 0, 0, width, height, w_flags);
  App::renderer = SDL_CreateRenderer(window, -1, r_flags);
  App::g_alive = true;

  SDL_Init(SDL_INIT_VIDEO);

  screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                             SDL_TEXTUREACCESS_TARGET, width, height);

  // Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer_Init(renderer);
}

/* Returns true if there is a pending event, and places it into the provided
 * struct. Otherwise returns false.
 * Note - this should be called every frame even if no events are needed.
 */
bool App::PollEvent(SDL_Event *e) {

  // Skip over events that ImGui wants to capture
  ImGuiIO &io = ImGui::GetIO();
  bool key_stolen, mouse_stolen;
  do {
    if (!SDL_PollEvent(e)) {
      return false;
    }

    ImGui_ImplSDL2_ProcessEvent(e);
    key_stolen = (e->type == SDL_KEYDOWN or e->type == SDL_KEYUP) and
                 io.WantCaptureKeyboard;
    mouse_stolen =
        (e->type >= SDL_MOUSEMOTION and e->type <= SDL_MOUSEWHEEL) and
        io.WantCaptureMouse;
  } while (key_stolen or mouse_stolen);

  // Event not used by ImGui, so we do App related stuff...
  switch (e->type) {
  case SDL_QUIT: {
    App::Quit();
  } break;
  case SDL_KEYDOWN: {
    switch (e->key.keysym.sym) {
    case SDLK_ESCAPE: {
      App::Quit();
    } break;
    }
  } break;
  }

  // ...then return to caller so they can use it
  return true;
}

void App::ClearScreen(SDL_Colour clear_colour) {
  auto [r, g, b, a] = clear_colour;
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  SDL_RenderClear(renderer);

  // Begin new ImGui frame
  ImGui_ImplSDLRenderer_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Any ImGui code called after this will show up on the next App::Present()
}

void App::Present() {
  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderCopy(renderer, screen, NULL, NULL);

  ImGui::Render();
  ImGuiIO &io = ImGui::GetIO();
  SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x,
                     io.DisplayFramebufferScale.y);
  ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}
