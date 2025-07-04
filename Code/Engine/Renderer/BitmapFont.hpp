#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
class Texture;
class Spline2D;
struct Vertex_PCU;
struct Vec2;
struct Rgba8;

//-----------------------------------------------------------------------------------------------
enum class TextBoxMode
{
	SHRINK_TO_FIT,
	OVERRUN
};


//-----------------------------------------------------------------------------------------------
class BitmapFont
{
	friend class Renderer; // Only the Renderer can create new BitmapFont Objects!

private:
	BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture);

public:
	Texture& GetTexture();
	void AddVertsForText2D(std::vector<Vertex_PCU>& verts, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::OPAQUE_WHITE, float cellAspectScale = 1.f);
	void AddVertsForTextInBox2D(std::vector<Vertex_PCU>& verts, std::string const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint = Rgba8::OPAQUE_WHITE, 
								float cellAspectScale = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), TextBoxMode mode = TextBoxMode::SHRINK_TO_FIT, int maxGlyphsToDraw = 99999999);
	void AddVertsForTextOnSpline2D(std::vector<Vertex_PCU>& verts, Spline2D const& spline, float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::OPAQUE_WHITE, float cellAspectScale = 1.f,
								float startOffsetAlongSpline = 0.f, float baselineOffset = 0.f) const;
	
	void AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& verts,
		float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::OPAQUE_WHITE,
		float cellAspect = 1.0f, Vec2 const& alignment = Vec2(0.5f, 0.5f),
		int maxGlyphsToDraw = 999);
	


	
	void GetInsertionPointForTextInBox2D(float& outInsertionPointHeight, Vec2& outInsertionPointBottomCenterPos, int insertionPointPosition, std::string const& text, AABB2 const& box, float cellHeight, 
		float cellAspectScale = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), TextBoxMode mode = TextBoxMode::SHRINK_TO_FIT);
	float GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale = 1.f);

protected:
	float GetGlyphAspect(int glyphUnicode) const; // For now this will always return m_fontDefaultAspect

protected:
	std::string m_fontFilePathNameWithNoExtension;
	SpriteSheet m_fontGlyphsSpriteSheet;
	float m_fontDefaultAspect = 1.0f;
};


/* Usage
// ...once, during initialization
BitmapFont* g_testFont = nullptr;
// ...and then, each frame; draw two text strings on screen
g_testFont = g_theRenderer->CreateOrGetBitmapFont( "Data/Fonts/MyFixedFont" ); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)

std::vector<Vertex_PCU> textVerts;
g_testFont->AddVertsForText2D( textVerts, Vec2( 100.f, 200.f ), 30.f, "Hello, world" );
g_testFont->AddVertsForText2D( textVerts, Vec2( 250.f, 400.f ), 15.f, "It's nice to have options!", Rgba8::RED, 0.6f );
g_theRenderer->BindTexture( &g_testFont->GetTexture() );
g_theRenderer->DrawVertexArray( textVerts );
*/