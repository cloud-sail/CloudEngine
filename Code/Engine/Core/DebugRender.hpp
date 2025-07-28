#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include <string>

//-----------------------------------------------------------------------------------------------
class Renderer;
class Camera;

struct Vec2;
struct Vec3;
struct Mat44;
struct AABB2;
//-----------------------------------------------------------------------------------------------
enum class DebugRenderMode
{
	ALWAYS,
	USE_DEPTH,
	X_RAY,
};

struct DebugRenderConfig
{
	Renderer* m_renderer = nullptr;
	std::string m_fontPath = "Data/Fonts/";
	std::string m_fontName = "SquirrelFixedFont";


	float m_messageCellHeight = 20.f; // 800 / 40 lines
	float m_messageAspectRatio = 0.7f;
};

// Setup
void DebugRenderSystemStartup(const DebugRenderConfig& config);
void DebugRenderSystemShutdown();

// Control
void DebugRenderSetVisible();
void DebugRenderSetHidden();
void DebugRenderClear();

// Output
void DebugRenderBeginFrame();
void DebugRenderWorld(Camera const& camera);
void DebugRenderScreen(Camera const& camera);
void DebugRenderEndFrame();

// Geometry
void DebugAddWorldWirePenumbraNoneCull(Vec3 const& center, Vec3 const& fwdNormal, float radius, float penumbraDot,
	float duration, Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldSphere(Vec3 const& center, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireSphereNoneCull(Vec3 const& center, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldCylinder(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireCylinder(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireArrow(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddBasis(Mat44 const& transform, float duration, float length, float radius,
	float colorScale = 1.0f, float alphaScale = 1.0f,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldBasis(Mat44 const& transform, float duration,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldText(std::string const& text, Mat44 const& transform, float textHeight, float duration, 
	float cellAspect = 1.0f, Vec2 const& alignment = Vec2(0.5f, 0.5f),
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldBillboardText(std::string const& text, Vec3 const& origin, float textHeight, float duration,
	float cellAspect = 1.0f, Vec2 const& alignment = Vec2(0.5f, 0.5f),
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddScreenText(std::string const& text, AABB2 const& box, float cellHeight,
	Vec2 const& alignment, float duration, float cellAspect = 1.0f,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE);
void DebugAddMessage(std::string const& text, float duration,
	Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE);

// Console commands
bool Command_DebugRenderClear(EventArgs& args); 
bool Command_DebugRenderToggle(EventArgs& args);
