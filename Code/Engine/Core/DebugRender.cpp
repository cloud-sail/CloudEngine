#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <vector>
#include <algorithm>

class Texture;

// only be seen in this cpp,  or use static for variable
namespace 
{
	constexpr float	MESSAGE_MARGIN_RATIO = 0.2f;

	DebugRenderConfig s_config;
	bool s_isVisible = true;
	BitmapFont* s_font = nullptr;

	struct DebugRenderObject
	{
	public:
		DebugRenderObject(Rgba8 const& startColor = Rgba8::OPAQUE_WHITE, Rgba8 const& endColor = Rgba8::OPAQUE_WHITE, float duration = -1.f, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

		void Render(Camera const& camera) const;
		bool IsFinished() const;
		bool IsPriorMessage() const;

	public:
		Rgba8 m_startColor = Rgba8::OPAQUE_WHITE;
		Rgba8 m_endColor = Rgba8::OPAQUE_WHITE;
		float m_duration = -1.f;
		DebugRenderMode m_mode = DebugRenderMode::USE_DEPTH;

		Timer m_timer;

		std::vector<Vertex_PCU> m_vertexs;
		Texture* m_texture = nullptr;
		RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
		bool m_isBillboard = false;
		//bool m_isScreenText = false; // not used?

		Vec3 m_origin; // billboard and text need this
	};

	DebugRenderObject::DebugRenderObject(Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, float duration /*= -1.f*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
		: m_startColor(startColor)
		, m_endColor(endColor)
		, m_duration(duration)
		, m_mode(mode)
	{
		if (m_duration >= 0.f)
		{
			m_timer.m_period = m_duration;
			m_timer.Start();
		}
	}

	void DebugRenderObject::Render(Camera const& camera) const
	{
		if (IsFinished())
		{
			return;
		}

		// Calculate the transform
		Mat44 transform;
		transform.SetTranslation3D(m_origin);
		if (m_isBillboard)
		{
			transform = GetBillboardTransform(BillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), m_origin);
		}


		// Calculate the color
		Rgba8 color;
		if (m_duration <= 0.f)
		{
			color = m_startColor;
		}
		else
		{
			color = Interpolate(m_startColor, m_endColor, GetClampedZeroToOne((float)m_timer.GetElapsedFraction()));
		}
		s_config.m_renderer->SetModelConstants(transform, color);
		s_config.m_renderer->BindTexture(m_texture); // font or white texture
		s_config.m_renderer->BindShader(nullptr);
		s_config.m_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		s_config.m_renderer->SetRasterizerMode(m_rasterizerMode); // some draw are wired

		if (m_mode == DebugRenderMode::ALWAYS)
		{
			s_config.m_renderer->SetBlendMode(BlendMode::ALPHA);
			s_config.m_renderer->SetDepthMode(DepthMode::DISABLED);
			s_config.m_renderer->DrawVertexArray(m_vertexs);
		}
		else if (m_mode == DebugRenderMode::USE_DEPTH)
		{
			s_config.m_renderer->SetBlendMode(BlendMode::ALPHA);
			s_config.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			s_config.m_renderer->DrawVertexArray(m_vertexs);

		}
		else if (m_mode == DebugRenderMode::X_RAY)
		{
			Rgba8 xRayColor(color.r, color.g, color.b, color.a / 2);
			s_config.m_renderer->SetModelConstants(transform, xRayColor);
			s_config.m_renderer->SetBlendMode(BlendMode::ALPHA);
			s_config.m_renderer->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
			s_config.m_renderer->DrawVertexArray(m_vertexs);

			s_config.m_renderer->SetModelConstants(transform, color);
			s_config.m_renderer->SetBlendMode(BlendMode::OPAQUE);
			s_config.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			s_config.m_renderer->DrawVertexArray(m_vertexs);
		}
	}

	bool DebugRenderObject::IsFinished() const
	{
		if (m_duration < 0.f)
		{
			return false;
		}
		return m_timer.HasPeriodElapsed();
	}

	bool DebugRenderObject::IsPriorMessage() const
	{
		// Infinite and One frame
		return m_duration <= 0.f;
	}

	std::vector<DebugRenderObject> s_worldGeometry;
	std::vector<DebugRenderObject> s_screenGeometry;
	std::vector<DebugRenderObject> s_screenMessage;

	void RemoveFinishedDebugRenderObjects(std::vector<DebugRenderObject>& vec)
	{
		vec.erase(
			std::remove_if(vec.begin(), vec.end(),
				[](DebugRenderObject const& obj) -> bool {return obj.IsFinished(); }),
			vec.end()
		);
	}
}

//-----------------------------------------------------------------------------------------------
#pragma region Setup
void DebugRenderSystemStartup(const DebugRenderConfig& config)
{
	s_config = config;
	g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderClear", Command_DebugRenderClear);
	g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderToggle", Command_DebugRenderToggle);
	s_font = s_config.m_renderer->CreateOrGetBitmapFont((s_config.m_fontPath + s_config.m_fontName).c_str());
}

void DebugRenderSystemShutdown()
{
	g_theEventSystem->UnsubscribeEventCallbackFunction("DebugRenderClear", Command_DebugRenderClear);
	g_theEventSystem->UnsubscribeEventCallbackFunction("DebugRenderToggle", Command_DebugRenderToggle);
}
#pragma endregion

//-----------------------------------------------------------------------------------------------
#pragma region Control

void DebugRenderSetVisible()
{
	s_isVisible = true;
}

void DebugRenderSetHidden()
{
	s_isVisible = false;
}

void DebugRenderClear()
{
	s_worldGeometry.clear();
	s_screenGeometry.clear();
	s_screenMessage.clear();
}
#pragma endregion

//-----------------------------------------------------------------------------------------------
#pragma region Output
void DebugRenderBeginFrame()
{
}

void DebugRenderWorld(Camera const& camera)
{
	if (!s_isVisible)
	{
		return;
	}

	s_config.m_renderer->BeginCamera(camera);
	s_config.m_renderer->BeginRenderEvent("DebugRenderWorld");
	int worldGeometryNum = static_cast<int>(s_worldGeometry.size());
	for (int index = 0; index < worldGeometryNum; ++index)
	{
		s_worldGeometry[index].Render(camera);
	}

	s_config.m_renderer->EndRenderEvent("DebugRenderWorld");
	s_config.m_renderer->EndCamera(camera);
}

void DebugRenderScreen(Camera const& camera)
{
	GUARANTEE_OR_DIE(camera.IsMode(Camera::Mode::eMode_Orthographic), "Debug Render Screen Camera is not orthographic!");
	if (!s_isVisible)
	{
		return;
	}

	s_config.m_renderer->BeginCamera(camera);
	s_config.m_renderer->BeginRenderEvent("DebugRenderScreen");
	//-----------------------------------------------------------------------------------------------
	int screenGeometryNum = static_cast<int>(s_screenGeometry.size());
	for (int index = 0; index < screenGeometryNum; ++index)
	{
		s_screenGeometry[index].Render(camera);
	}

	//-----------------------------------------------------------------------------------------------
	int messageSlot = 1;
	int screenMessageNum = static_cast<int>(s_screenMessage.size());
	AABB2 cameraBounds = AABB2(camera.GetOrthographicBottomLeft(), camera.GetOrthographicTopRight());
	float const messageMargin = s_config.m_messageCellHeight * MESSAGE_MARGIN_RATIO;
	// First Display infinite duration messages
	for (int index = 0; index < screenMessageNum; ++index)
	{
		DebugRenderObject& currentMessage = s_screenMessage[index];
		if (currentMessage.IsPriorMessage() && !currentMessage.IsFinished())
		{
			currentMessage.m_origin = Vec3(cameraBounds.m_mins.x + messageMargin, 
				cameraBounds.m_maxs.y - static_cast<float>(messageSlot) * (s_config.m_messageCellHeight + 2.f * messageMargin), 
				0.f);
			messageSlot++;
		}
	}

	//  Then Display message which is not finished
	for (int index = 0; index < screenMessageNum; ++index)
	{
		DebugRenderObject& currentMessage = s_screenMessage[index];
		if (!currentMessage.IsPriorMessage() && !currentMessage.IsFinished())
		{
			currentMessage.m_origin = Vec3(cameraBounds.m_mins.x + messageMargin,
				cameraBounds.m_maxs.y - static_cast<float>(messageSlot) * (s_config.m_messageCellHeight + 2.f * messageMargin),
				0.f);
			messageSlot++;
		}
	}

	for (int index = 0; index < screenMessageNum; ++index)
	{
		s_screenMessage[index].Render(camera);
	}

	//-----------------------------------------------------------------------------------------------
	s_config.m_renderer->EndRenderEvent("DebugRenderScreen");
	s_config.m_renderer->EndCamera(camera);
}

void DebugRenderEndFrame()
{
	RemoveFinishedDebugRenderObjects(s_worldGeometry);
	RemoveFinishedDebugRenderObjects(s_screenGeometry);
	RemoveFinishedDebugRenderObjects(s_screenMessage);
}


#pragma endregion

//-----------------------------------------------------------------------------------------------
#pragma region Geometry
void DebugAddWorldWirePenumbraNoneCull(Vec3 const& center, Vec3 const& fwdNormal, float radius, float penumbraDot, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForPenumbra3D(obj.m_vertexs, center, fwdNormal, radius, penumbraDot);
	obj.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE;
}

void DebugAddWorldSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForSphere3D(obj.m_vertexs, center, radius);
}

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForSphere3D(obj.m_vertexs, center, radius);
	obj.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;
}

void DebugAddWorldWireSphereNoneCull(Vec3 const& center, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForSphere3D(obj.m_vertexs, center, radius);
	obj.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE;
}

void DebugAddWorldCylinder(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForCylinder3D(obj.m_vertexs, start, end, radius);
}

void DebugAddWorldWireCylinder(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForCylinder3D(obj.m_vertexs, start, end, radius);
	obj.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;
}

void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForArrow3D(obj.m_vertexs, start, end, radius);
}

void DebugAddWorldWireArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	AddVertsForArrow3D(obj.m_vertexs, start, end, radius);
	obj.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;
}

void DebugAddBasis(Mat44 const& transform, float duration, float length, float radius, float colorScale /*= 1.0f*/, float alphaScale /*= 1.0f*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(Rgba8::OPAQUE_WHITE, Rgba8::OPAQUE_WHITE, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();

	Vec3 iBasis = transform.GetIBasis3D();
	Vec3 jBasis = transform.GetJBasis3D();
	Vec3 kBasis = transform.GetKBasis3D();
	Vec3 translation = transform.GetTranslation3D();

	unsigned char colorValue = DenormalizeByte(colorScale);
	unsigned char alphaValue = DenormalizeByte(alphaScale);

	AddVertsForArrow3D(obj.m_vertexs, translation, translation + iBasis * length, radius, Rgba8(colorValue, 0, 0, alphaValue));
	AddVertsForArrow3D(obj.m_vertexs, translation, translation + jBasis * length, radius, Rgba8(0, colorValue, 0, alphaValue));
	AddVertsForArrow3D(obj.m_vertexs, translation, translation + kBasis * length, radius, Rgba8(0, 0, colorValue, alphaValue));
}

void DebugAddWorldBasis(Mat44 const& transform, float duration, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	constexpr float LENGTH = 1.f;
	constexpr float RADIUS = 0.075f;
	s_worldGeometry.emplace_back(Rgba8::OPAQUE_WHITE, Rgba8::OPAQUE_WHITE, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();

	Vec3 iBasis = transform.GetIBasis3D();
	Vec3 jBasis = transform.GetJBasis3D();
	Vec3 kBasis = transform.GetKBasis3D();
	Vec3 translation = transform.GetTranslation3D();

	AddVertsForArrow3D(obj.m_vertexs, translation, translation + iBasis * LENGTH, RADIUS, Rgba8::RED);
	AddVertsForArrow3D(obj.m_vertexs, translation, translation + jBasis * LENGTH, RADIUS, Rgba8::GREEN);
	AddVertsForArrow3D(obj.m_vertexs, translation, translation + kBasis * LENGTH, RADIUS, Rgba8::BLUE);
}

void DebugAddWorldText(std::string const& text, Mat44 const& transform, float textHeight, float duration, float cellAspect /*= 1.0f*/, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	s_font->AddVertsForText3DAtOriginXForward(obj.m_vertexs, textHeight, text, Rgba8::OPAQUE_WHITE, cellAspect, alignment);
	TransformVertexArray3D(obj.m_vertexs, transform);
	obj.m_texture = &s_font->GetTexture();
	obj.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
}

void DebugAddWorldBillboardText(std::string const& text, Vec3 const& origin, float textHeight, float duration, float cellAspect /*= 1.0f*/, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	s_worldGeometry.emplace_back(startColor, endColor, duration, mode);
	DebugRenderObject& obj = s_worldGeometry.back();
	s_font->AddVertsForText3DAtOriginXForward(obj.m_vertexs, textHeight, text, Rgba8::OPAQUE_WHITE, cellAspect, alignment);
	obj.m_texture = &s_font->GetTexture();
	obj.m_isBillboard = true;
	obj.m_origin = origin;
}


void DebugAddScreenText(std::string const& text, AABB2 const& box, float cellHeight, Vec2 const& alignment, float duration, float cellAspect /*= 1.0f*/, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/)
{
	s_screenGeometry.emplace_back(startColor, endColor, duration, DebugRenderMode::ALWAYS);
	DebugRenderObject& obj = s_screenGeometry.back();
	s_font->AddVertsForTextInBox2D(obj.m_vertexs, text, box, cellHeight, Rgba8::OPAQUE_WHITE, cellAspect, alignment);
	obj.m_texture = &s_font->GetTexture();
	//obj.m_isScreenText = true;
}

void DebugAddMessage(std::string const& text, float duration, Rgba8 const& startColor /*= Rgba8::OPAQUE_WHITE*/, Rgba8 const& endColor /*= Rgba8::OPAQUE_WHITE*/)
{
	s_screenMessage.emplace_back(startColor, endColor, duration, DebugRenderMode::ALWAYS);
	DebugRenderObject& obj = s_screenMessage.back();
	s_font->AddVertsForText2D(obj.m_vertexs, Vec2::ZERO, s_config.m_messageCellHeight, text, Rgba8::OPAQUE_WHITE, s_config.m_messageAspectRatio);
	obj.m_texture = &s_font->GetTexture();
	//obj.m_isScreenText = true;
}
#pragma endregion

//-----------------------------------------------------------------------------------------------
bool Command_DebugRenderClear(EventArgs& args)
{
	UNUSED(args);
	DebugRenderClear();
	return true;
}

bool Command_DebugRenderToggle(EventArgs& args)
{
	UNUSED(args);
	if (s_isVisible)
	{
		DebugRenderSetHidden();
	}
	else
	{
		DebugRenderSetVisible();
	}
	return true;
}