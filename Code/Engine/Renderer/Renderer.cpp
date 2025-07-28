#include "Engine/Renderer/Renderer.hpp"

Renderer* Renderer::s_mainRenderer = nullptr;

const std::string WINDOW_RESIZE_EVENT = "WindowResized";
