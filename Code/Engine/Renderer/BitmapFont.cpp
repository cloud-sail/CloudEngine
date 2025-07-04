#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Spline.hpp"

// Bitmap 16x16.png "tier 1" bitmap spritesheet layout
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture)
	: m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
	, m_fontGlyphsSpriteSheet(fontTexture, IntVec2(16, 16))
{
}

Texture& BitmapFont::GetTexture()
{
	return m_fontGlyphsSpriteSheet.GetTexture();
}

void BitmapFont::AddVertsForText2D(std::vector<Vertex_PCU>& verts, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint /*= Rgba8::OPAQUE_WHITE*/, float cellAspectScale /*= 1.f*/)
{
	// TODO if the length of character is not 1-byte, we create new method?
	float currentOffsetX = 0.f;

	for (size_t i = 0; i < text.length(); ++i)
	{
		int glyphUnicode = static_cast<int>(text[i]); // that is also spriteIndex in ascii
		float glyphAspect = GetGlyphAspect(glyphUnicode);
		float cellWidth = cellHeight * glyphAspect * cellAspectScale;

		Vec2 offset(currentOffsetX, 0.f);
		AABB2 bounds = AABB2(textMins + offset, textMins + Vec2(cellWidth, cellHeight) + offset);
		AABB2 UVs = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphUnicode);
		AddVertsForAABB2D(verts, bounds, tint, UVs);

		currentOffsetX += cellWidth;
	}
}

// if alignment is outside of ZERO_TO_ONE
void BitmapFont::AddVertsForTextInBox2D(std::vector<Vertex_PCU>& verts, std::string const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint /*= Rgba8::OPAQUE_WHITE*/, float cellAspectScale /*= 1.f*/, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, TextBoxMode mode /*= TextBoxMode::SHRINK_TO_FIT*/, int maxGlyphsToDraw /*= 99999999*/)
{
	Strings textLines = SplitStringOnDelimiter(text, '\n');

	float adjustedCellHeight = cellHeight;
	
	if (mode == TextBoxMode::SHRINK_TO_FIT)
	{
		float paragraphHeight = static_cast<float>(textLines.size()) * cellHeight;
		float paragraphWidth = 0.f;
		for (size_t lineIndex = 0; lineIndex < textLines.size(); ++lineIndex)
		{
			float lineWidth = GetTextWidth(cellHeight, textLines[lineIndex], cellAspectScale);
			if (lineWidth > paragraphWidth)
			{
				paragraphWidth = lineWidth;
			}
		}
		if (paragraphWidth > box.GetDimensions().x || paragraphHeight > box.GetDimensions().y)
		{
			float widthRatio = box.GetDimensions().x / paragraphWidth;
			float HeightRatio = box.GetDimensions().y / paragraphHeight;
			float minRatio = (widthRatio < HeightRatio) ? widthRatio : HeightRatio;
			adjustedCellHeight *= minRatio;
		}
	}

	std::vector<AABB2> lineBoxs;
	lineBoxs.reserve(textLines.size());
	for (size_t lineIndex = 0; lineIndex < textLines.size(); ++lineIndex)
	{
		float lineWidth = GetTextWidth(adjustedCellHeight, textLines[lineIndex], cellAspectScale);
		lineBoxs.push_back(AABB2(Vec2::ZERO, Vec2(lineWidth, adjustedCellHeight)));
	}

	// the pivot of All Boxes are the same
	float const boxPivotX = Interpolate(box.m_mins.x, box.m_maxs.x, alignment.x);
	float const boxPivotY = Interpolate(box.m_mins.y, box.m_maxs.y, alignment.y);

	float paragraphHeight = static_cast<float>(textLines.size()) * adjustedCellHeight;

	for (size_t boxIndex = 0; boxIndex < lineBoxs.size(); ++boxIndex)
	{
		AABB2& lineBox = lineBoxs[boxIndex];
		Vec2 translation;
		float lineBoxPivotX = Interpolate(lineBox.m_mins.x, lineBox.m_maxs.x, alignment.x);
		translation.x = boxPivotX - lineBoxPivotX;
		
		translation.y = boxPivotY - adjustedCellHeight * static_cast<float>(boxIndex + 1) + (1.f - alignment.y) * paragraphHeight;

		lineBox.Translate(translation);
	}


	// Final step: after moving and resizing the box, add verts with in maxGlyphsToDraw
	int glyphsDrawn = 0;
	for (size_t boxIndex = 0; boxIndex < lineBoxs.size(); ++boxIndex)
	{
		AABB2& lineBox = lineBoxs[boxIndex];
		std::string lineText = textLines[boxIndex];
		int lineTextLength = static_cast<int>(lineText.length());

		int remainingGlyphs = maxGlyphsToDraw - glyphsDrawn;
		if (remainingGlyphs <= 0)
		{
			break;
		}
		if (lineTextLength <= remainingGlyphs)
		{
			AddVertsForText2D(verts, lineBox.m_mins, adjustedCellHeight, lineText, tint, cellAspectScale);
			glyphsDrawn += lineTextLength;
		}
		else
		{
			std::string cutText = lineText.substr(0, remainingGlyphs);
			AddVertsForText2D(verts, lineBox.m_mins, adjustedCellHeight, cutText, tint, cellAspectScale);
			glyphsDrawn += remainingGlyphs;
			break;
		}
	}
}

void BitmapFont::AddVertsForTextOnSpline2D(std::vector<Vertex_PCU>& verts, Spline2D const& spline, float cellHeight, std::string const& text, Rgba8 const& tint /*= Rgba8::OPAQUE_WHITE*/, float cellAspectScale /*= 1.f*/, float startOffsetAlongSpline /*= 0.f*/, float baselineOffset /*= 0.f*/) const
{
	float maxSplineLength = spline.GetSplineLength();
	float currentDistAlongSpline = startOffsetAlongSpline;

	int numChars = (int)text.length();
	for (int i = 0; i < numChars; ++i)
	{
		int glyphUnicode = static_cast<int>(text[i]); // that is also spriteIndex in ascii
		float glyphAspect = GetGlyphAspect(glyphUnicode);
		float cellWidth = cellHeight * glyphAspect * cellAspectScale;

		// First Half
		currentDistAlongSpline += 0.5f * cellWidth;
		if (currentDistAlongSpline > maxSplineLength)
		{
			break;
		}

		float splineInputKey = spline.GetInputKeyAtDistanceAlongSpline(currentDistAlongSpline);
		Vec2 splinePointPos = spline.GetPositionAtInputKey(splineInputKey);
		Vec2 iBasis = spline.GetDirectionAtInputKey(splineInputKey);
		Vec2 jBasis = iBasis.GetRotated90Degrees();

		Vec2 basePoint = splinePointPos + jBasis * baselineOffset;

		Vec2 BL = basePoint - iBasis * (0.5f * cellWidth);
		Vec2 BR = basePoint + iBasis * (0.5f * cellWidth);
		Vec2 TR = BR + jBasis * cellHeight;
		Vec2 TL = BL + jBasis * cellHeight;

		AABB2 UVs = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphUnicode);

		AddVertsForQuad2D(verts, BL, BR, TR, TL, tint, UVs);
		// Second Half
		currentDistAlongSpline += 0.5f * cellWidth;
	}
}

void BitmapFont::AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& verts, float cellHeight, std::string const& text, Rgba8 const& tint /*= Rgba8::OPAQUE_WHITE*/, float cellAspect /*= 1.0f*/, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, int maxGlyphsToDraw /*= 999*/)
{
	std::string cutText = text.substr(0, maxGlyphsToDraw);

	Vec3 iBasis = Vec3(0.f, 1.f, 0.f);
	Vec3 jBasis = Vec3(0.f, 0.f, 1.f);
	Vec3 kBasis = Vec3(1.f, 0.f, 0.f);

	std::vector<Vertex_PCU> textVerts;
	AddVertsForText2D(textVerts, Vec2::ZERO, cellHeight, cutText, tint, cellAspect);

	float width = GetTextWidth(cellHeight, cutText, cellAspect);
	float height = cellHeight;

	Vec3 translation = Vec3(0.f, -alignment.x * width, -alignment.y * height);
	Mat44 transform = Mat44(iBasis, jBasis, kBasis, translation);

	TransformVertexArray3D(textVerts, transform);

	verts.reserve(verts.size() + textVerts.size());

	for (Vertex_PCU const& vertex : textVerts)
	{
		verts.push_back(vertex);
	}
}

void BitmapFont::GetInsertionPointForTextInBox2D(float& outInsertionPointHeight, Vec2& outInsertionPointBottomCenterPos, int insertionPointPosition, std::string const& text, AABB2 const& box, float cellHeight, float cellAspectScale /*= 1.f*/, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, TextBoxMode mode /*= TextBoxMode::SHRINK_TO_FIT*/)
{
	GUARANTEE_OR_DIE(insertionPointPosition <= text.length() && insertionPointPosition >= 0, "Insertion Point Position Out of Range!");
	Strings textLines = SplitStringOnDelimiter(text, '\n');

	float adjustedCellHeight = cellHeight;

	if (mode == TextBoxMode::SHRINK_TO_FIT)
	{
		float paragraphHeight = static_cast<float>(textLines.size()) * cellHeight;
		float paragraphWidth = 0.f;
		for (size_t lineIndex = 0; lineIndex < textLines.size(); ++lineIndex)
		{
			float lineWidth = GetTextWidth(cellHeight, textLines[lineIndex], cellAspectScale);
			if (lineWidth > paragraphWidth)
			{
				paragraphWidth = lineWidth;
			}
		}
		if (paragraphWidth > box.GetDimensions().x || paragraphHeight > box.GetDimensions().y)
		{
			float widthRatio = box.GetDimensions().x / paragraphWidth;
			float HeightRatio = box.GetDimensions().y / paragraphHeight;
			float minRatio = (widthRatio < HeightRatio) ? widthRatio : HeightRatio;
			adjustedCellHeight *= minRatio;
		}
	}

	std::vector<AABB2> lineBoxs;
	lineBoxs.reserve(textLines.size());
	for (size_t lineIndex = 0; lineIndex < textLines.size(); ++lineIndex)
	{
		float lineWidth = GetTextWidth(adjustedCellHeight, textLines[lineIndex], cellAspectScale);
		lineBoxs.push_back(AABB2(Vec2::ZERO, Vec2(lineWidth, adjustedCellHeight)));
	}

	// the pivot of All Boxes are the same
	float const boxPivotX = Interpolate(box.m_mins.x, box.m_maxs.x, alignment.x);
	float const boxPivotY = Interpolate(box.m_mins.y, box.m_maxs.y, alignment.y);

	float paragraphHeight = static_cast<float>(textLines.size()) * adjustedCellHeight;

	for (size_t boxIndex = 0; boxIndex < lineBoxs.size(); ++boxIndex)
	{
		AABB2& lineBox = lineBoxs[boxIndex];
		Vec2 translation;
		float lineBoxPivotX = Interpolate(lineBox.m_mins.x, lineBox.m_maxs.x, alignment.x);
		translation.x = boxPivotX - lineBoxPivotX;

		translation.y = boxPivotY - adjustedCellHeight * static_cast<float>(boxIndex + 1) + (1.f - alignment.y) * paragraphHeight;

		lineBox.Translate(translation);
	}


	// Final step: after moving and resizing the box, find insertion point position
	outInsertionPointHeight = adjustedCellHeight;
	outInsertionPointBottomCenterPos = Vec2();

	int remainingInsertionPointPosition = insertionPointPosition;
	for (size_t boxIndex = 0; boxIndex < lineBoxs.size(); ++boxIndex)
	{
		AABB2& lineBox = lineBoxs[boxIndex];
		std::string lineText = textLines[boxIndex];
		int lineTextLength = static_cast<int>(lineText.length());

		// insertion point is in current line
		if (remainingInsertionPointPosition <= lineTextLength)
		{
			std::string cutText = lineText.substr(0, remainingInsertionPointPosition);
			float offset = GetTextWidth(adjustedCellHeight, cutText, cellAspectScale);
			outInsertionPointBottomCenterPos = lineBox.m_mins + Vec2(offset, 0.f);
			return;
		}

		// next line
		remainingInsertionPointPosition -= (lineTextLength + 1);
	}
}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale /*= 1.f*/)
{
	// TODO if the length of character is not 1-byte, we create new method?
	float totalWidth = 0.f;
	for (size_t i = 0; i < text.length(); ++i)
	{
		int glyphUnicode = static_cast<int>(text[i]);
		float glyphAspect = GetGlyphAspect(glyphUnicode);
		float cellWidth = cellHeight * glyphAspect * cellAspectScale;
		totalWidth += cellWidth;
	}

	return totalWidth;
}

float BitmapFont::GetGlyphAspect(int glyphUnicode) const
{
	UNUSED(glyphUnicode);

	return m_fontDefaultAspect;
}
