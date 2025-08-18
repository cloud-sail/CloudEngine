#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"


class Ray3;

class Camera
{
public:
	enum Mode
	{
		eMode_Orthographic,
		eMode_Perspective,

		eMode_Count
	};

	void SetOrthographicView(Vec2 const& bottomLeft, Vec2 const& topRight, float near = 0.0f, float far = 1.0f);
	void SetPerspectiveView(float aspect, float fov, float near, float far);

	void SetPositionAndOrientation(Vec3 const& position, EulerAngles const& orientation);
	void SetPosition(Vec3 const& position);
	Vec3 GetPosition() const;
	void SetOrientation(EulerAngles const& orientation);
	EulerAngles GetOrientation() const;

	Mat44 GetCameraToWorldTransform() const;
	Mat44 GetWorldToCameraTransform() const;

	void SetCameraToRenderTransform(Mat44 const& m);
	Mat44 GetCameraToRenderTransform() const;

	Mat44 GetRenderToClipTransform() const;

	Vec2 GetOrthographicBottomLeft() const;
	Vec2 GetOrthographicTopRight() const;
	void Translate2D(Vec2 const& translation2D);

	Mat44 GetOrthographicMatrix() const;
	Mat44 GetPerspectiveMatrix() const;
	Mat44 GetProjectionMatrix() const;

	Mat44 GetClipToWorldTransform() const;

	bool IsMode(Mode mode) const;

	void SetNormalizedViewPort(AABB2 normalizedViewPort);
	void GetDirectXViewport(Vec2 const& clientDimensions, float& topLeftX, float& topLeftY, float& width, float& height) const;

	bool ProjectWorldToScreenPoint(Vec3 const& worldPos, Vec2& out_screenPos) const;
	bool ProjectWorldToViewportPoint(Vec3 const& worldPos, Vec2& out_viewportPos) const;

	bool ScreenPointToRay(Vec3& out_rayStart, Vec3& out_rayFwdNormal, Vec2 const& clientUV) const;

	// Alias Function
	void SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight);
	Vec2 GetOrthoBottomLeft() const;
	Vec2 GetOrthoTopRight() const;

protected:
	Mode m_mode = eMode_Orthographic;

	Vec3 m_position;
	EulerAngles m_orientation;
	
	Vec2 m_orthographicBottomLeft;
	Vec2 m_orthographicTopRight;
	float m_orthographicNear;
	float m_orthographicFar;

	float m_perspectiveAspect;
	float m_perspectiveFOV;
	float m_perspectiveNear;
	float m_perspectiveFar;

	//Mat44 m_cameraToRenderTransform = Mat44::DIRECTX_C2R;
	Mat44 m_cameraToRenderTransform;

	AABB2 m_normalizedViewport = AABB2::ZERO_TO_ONE;

};
