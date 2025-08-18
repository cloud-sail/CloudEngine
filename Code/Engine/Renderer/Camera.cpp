#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Window/Window.hpp"


void Camera::SetOrthographicView(Vec2 const& bottomLeft, Vec2 const& topRight, float near /*= 0.0f*/, float far /*= 1.0f*/)
{
	m_mode = eMode_Orthographic;
	m_orthographicBottomLeft = bottomLeft;
	m_orthographicTopRight = topRight;
	m_orthographicNear = near;
	m_orthographicFar = far;
}

void Camera::SetPerspectiveView(float aspect, float fov, float near, float far)
{
	m_mode = eMode_Perspective;
	m_perspectiveAspect = aspect;
	m_perspectiveFOV = fov;
	m_perspectiveNear = near;
	m_perspectiveFar = far;
}

void Camera::SetPositionAndOrientation(Vec3 const& position, EulerAngles const& orientation)
{
	m_position = position;
	m_orientation = orientation;
}

void Camera::SetPosition(Vec3 const& position)
{
	m_position = position;
}

Vec3 Camera::GetPosition() const
{
	return m_position;
}

void Camera::SetOrientation(EulerAngles const& orientation)
{
	m_orientation = orientation;
}

EulerAngles Camera::GetOrientation() const
{
	return m_orientation;
}

Mat44 Camera::GetCameraToWorldTransform() const
{
	Mat44 result = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	result.SetTranslation3D(m_position);
	return result;
}

Mat44 Camera::GetWorldToCameraTransform() const
{
	Mat44 result = GetCameraToWorldTransform();
	return result.GetOrthonormalInverse();
	//Mat44 result = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	//result.TransposeIJK();
	//result.Append(Mat44::MakeTranslation3D(-m_position));
	//return result;
}

void Camera::SetCameraToRenderTransform(Mat44 const& m)
{
	// Must be supplied by game code when configuring the camera.
	m_cameraToRenderTransform = m;
}

Mat44 Camera::GetCameraToRenderTransform() const
{
	return m_cameraToRenderTransform;
}

Mat44 Camera::GetRenderToClipTransform() const
{
	return GetProjectionMatrix();
}

Vec2 Camera::GetOrthographicBottomLeft() const
{
	return m_orthographicBottomLeft;
}

Vec2 Camera::GetOrthographicTopRight() const
{
	return m_orthographicTopRight;
}

void Camera::Translate2D(Vec2 const& translation2D)
{
	m_orthographicBottomLeft += translation2D;
	m_orthographicTopRight += translation2D;
}

Mat44 Camera::GetOrthographicMatrix() const
{
	return Mat44::MakeOrthoProjection(m_orthographicBottomLeft.x, m_orthographicTopRight.x, m_orthographicBottomLeft.y, m_orthographicTopRight.y, m_orthographicNear, m_orthographicFar);
}

Mat44 Camera::GetPerspectiveMatrix() const
{
	return Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);
}

Mat44 Camera::GetProjectionMatrix() const
{
	if (m_mode == eMode_Orthographic)
	{
		return GetOrthographicMatrix();
	}
	if (m_mode == eMode_Perspective)
	{
		return GetPerspectiveMatrix();
	}

	return Mat44();
}

Mat44 Camera::GetClipToWorldTransform() const
{
	Mat44 result;
	// Get World To Clip
	result.Append(GetRenderToClipTransform());
	result.Append(GetCameraToRenderTransform());
	result.Append(GetWorldToCameraTransform());
	// Inverse: clip to world
	result.Inverse();
	return result;
}

bool Camera::IsMode(Mode mode) const
{
	return m_mode == mode;
}

void Camera::SetNormalizedViewPort(AABB2 normalizedViewPort)
{
	m_normalizedViewport = normalizedViewPort;
}

void Camera::GetDirectXViewport(Vec2 const& clientDimensions, float& topLeftX, float& topLeftY, float& width, float& height) const
{
	topLeftX = clientDimensions.x * m_normalizedViewport.m_mins.x;
	topLeftY = clientDimensions.y * (1.f - m_normalizedViewport.m_maxs.y);
	width = clientDimensions.x * (m_normalizedViewport.m_maxs.x - m_normalizedViewport.m_mins.x);
	height = clientDimensions.y * (m_normalizedViewport.m_maxs.y - m_normalizedViewport.m_mins.y);

}

bool Camera::ProjectWorldToScreenPoint(Vec3 const& worldPos, Vec2& out_screenPos) const
{
	Mat44 const WorldToCameraTransform = GetWorldToCameraTransform();	// View Transform
	Mat44 const CameraToRenderTransform = GetCameraToRenderTransform();	// Non-standard transform
	Mat44 const RenderToClipTransform = GetRenderToClipTransform();	// Projection transform

	Vec4 worldSpacePosition = Vec4(worldPos.x, worldPos.y, worldPos.z, 1.f);
	Mat44 worldToClipTransform = RenderToClipTransform;
	worldToClipTransform.Append(CameraToRenderTransform);
	worldToClipTransform.Append(WorldToCameraTransform);

	Vec4 clipSpacePosition = worldToClipTransform.TransformHomogeneous3D(worldSpacePosition);

	bool isInsideView = clipSpacePosition.w > 0.f;
	if (!isInsideView)
	{
		return false;
	}

	float invW = 1.f / clipSpacePosition.w;
	Vec2 ndcSpacePosition = Vec2(clipSpacePosition.x * invW, clipSpacePosition.y * invW);
	Vec2 clientDimensions = Vec2(Window::s_mainWindow->GetClientDimensions());

	AABB2 viewportBoundsInScreenCoords = AABB2(clientDimensions * m_normalizedViewport.m_mins, clientDimensions * m_normalizedViewport.m_maxs);

	Vec2 uv = Vec2(ndcSpacePosition.x * 0.5f + 0.5f, ndcSpacePosition.y * 0.5f + 0.5f);
	out_screenPos = viewportBoundsInScreenCoords.GetPointAtUV(uv);
	return true;
}

bool Camera::ProjectWorldToViewportPoint(Vec3 const& worldPos, Vec2& out_viewportPos) const
{
	// The Viewport space is normalized and relative to the camera
	Mat44 const WorldToCameraTransform = GetWorldToCameraTransform();	// View Transform
	Mat44 const CameraToRenderTransform = GetCameraToRenderTransform();	// Non-standard transform
	Mat44 const RenderToClipTransform = GetRenderToClipTransform();	// Projection transform

	Vec4 worldSpacePosition = Vec4(worldPos.x, worldPos.y, worldPos.z, 1.f);
	Mat44 worldToClipTransform = RenderToClipTransform;
	worldToClipTransform.Append(CameraToRenderTransform);
	worldToClipTransform.Append(WorldToCameraTransform);

	Vec4 clipSpacePosition = worldToClipTransform.TransformHomogeneous3D(worldSpacePosition);

	bool isInsideView = clipSpacePosition.w > 0.f;
	if (!isInsideView)
	{
		return false;
	}

	float invW = 1.f / clipSpacePosition.w;
	Vec2 ndcSpacePosition = Vec2(clipSpacePosition.x * invW, clipSpacePosition.y * invW);
	
	out_viewportPos = Vec2(ndcSpacePosition.x * 0.5f + 0.5f, ndcSpacePosition.y * 0.5f + 0.5f);
	return true;
}


bool Camera::ScreenPointToRay(Vec3& out_rayStart, Vec3& out_rayFwdNormal, Vec2 const& clientUV) const
{
	GUARANTEE_OR_DIE(m_mode == eMode_Perspective, "The camera mode is not perspective.");
	if (!m_normalizedViewport.IsPointInside(clientUV))
	{
		return false; // invalid ray
	}

	Vec2 viewportUV = m_normalizedViewport.GetUVForPoint(clientUV);
	Vec2 viewportNormalizedCoordinate = Vec2(2.f * viewportUV.x - 1.f, 2.f * viewportUV.y - 1.f);

	float tanHalfFov = TanDegrees(0.5f * m_perspectiveFOV);
	
	Vec3 localPoint;
	localPoint.x = 1.f;
	localPoint.y = tanHalfFov * m_perspectiveAspect * -viewportNormalizedCoordinate.x;
	localPoint.z = tanHalfFov * viewportNormalizedCoordinate.y;

	Mat44 rot = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();

	Vec3 direction = rot.TransformVectorQuantity3D(localPoint);

	out_rayStart = m_position;
	out_rayFwdNormal = direction.GetNormalized();
	return true;
}

//-----------------------------------------------------------------------------------------------
void Camera::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight)
{
	SetOrthographicView(bottomLeft, topRight);
}

Vec2 Camera::GetOrthoBottomLeft() const
{
	return m_orthographicBottomLeft;
}

Vec2 Camera::GetOrthoTopRight() const
{
	return m_orthographicTopRight;
}

