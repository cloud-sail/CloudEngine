#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Frustum.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/VertexUtils.hpp"


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

Frustum Camera::GetFrustum() const
{
	if (m_mode == eMode_Orthographic)
	{
		return GetOrthographicFrustum();
	}
	if (m_mode == eMode_Perspective)
	{
		return GetPerspectiveFrustum();
	}

	return Frustum();
}

void Camera::DebugDrawFrustum() const
{
	if (m_mode == eMode_Perspective)
	{
		DebugDrawPerspectiveFrustum();
		return;
	}
	if (m_mode == eMode_Orthographic)
	{
		DebugDrawOrthographicFrustum();
		return;
	}
}

Frustum Camera::GetOrthographicFrustum() const
{
	// Remember to make m_cameraToRenderTransform = Mat44::DIRECTX_C2R;
	Frustum f;

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);

	Vec3 nearPlaneOrigin = m_position + forwardIBasis * m_orthographicNear;
	Vec3 farPlaneOrigin = m_position + forwardIBasis * m_orthographicFar;

	Vec3 farPlaneBottomLeft = farPlaneOrigin - leftJBasis * m_orthographicBottomLeft.x + upKBasis * m_orthographicBottomLeft.y;
	Vec3 farPlaneTopRight = farPlaneOrigin - leftJBasis * m_orthographicTopRight.x + upKBasis * m_orthographicTopRight.y;


	f.m_nearFace = Plane3(forwardIBasis, nearPlaneOrigin);
	f.m_farFace = Plane3(-forwardIBasis, farPlaneOrigin);

	f.m_leftFace = Plane3(-leftJBasis, farPlaneBottomLeft);
	f.m_rightFace = Plane3(leftJBasis, farPlaneTopRight);

	f.m_topFace = Plane3(-upKBasis, farPlaneTopRight);
	f.m_bottomFace = Plane3(upKBasis, farPlaneBottomLeft);

	return f;
}

Frustum Camera::GetPerspectiveFrustum() const
{
	Frustum f;

	// V-Vertical H-Horizontal
	const float halfVSide = m_perspectiveFar * TanDegrees(m_perspectiveFOV * 0.5f);
	const float halfHSide = halfVSide * m_perspectiveAspect;

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);

	Vec3 nearPlaneForward = forwardIBasis * m_perspectiveNear;
	Vec3 farPlaneForward = forwardIBasis * m_perspectiveFar;

	f.m_nearFace = Plane3(forwardIBasis, m_position + nearPlaneForward);
	f.m_farFace = Plane3(-forwardIBasis, m_position + farPlaneForward);

	f.m_leftFace = Plane3(CrossProduct3D(farPlaneForward + leftJBasis * halfHSide, upKBasis).GetNormalized(), m_position);
	f.m_rightFace = Plane3(CrossProduct3D(upKBasis, farPlaneForward - leftJBasis * halfHSide).GetNormalized(), m_position);

	f.m_topFace = Plane3(CrossProduct3D(leftJBasis, farPlaneForward + upKBasis * halfVSide).GetNormalized(), m_position);
	f.m_bottomFace = Plane3(CrossProduct3D(farPlaneForward - upKBasis * halfVSide, leftJBasis).GetNormalized(), m_position);

	return f;
}

void Camera::DebugDrawPerspectiveFrustum() const
{
	constexpr int NUM_SLICES = 25;
	const Rgba8 MAJOR_COLOR(255, 0, 0);
	const Rgba8 MINOR_COLOR(100, 60, 60);

	const float tanHalfFOV = TanDegrees(m_perspectiveFOV * 0.5f);

	const float nearHalfVSide = m_perspectiveNear * tanHalfFOV;
	const float nearHalfHSide = nearHalfVSide * m_perspectiveAspect;

	const float farHalfVSide = m_perspectiveFar * tanHalfFOV;
	const float farHalfHSide = farHalfVSide * m_perspectiveAspect;

	// Draw frustum in world space
	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);

	Vec3 nearPlaneForward = forwardIBasis * m_perspectiveNear;
	Vec3 farPlaneForward = forwardIBasis * m_perspectiveFar;

	const Vec3 nearHalfVOffset = upKBasis * nearHalfVSide;
	const Vec3 nearHalfHOffset = leftJBasis * nearHalfHSide;

	const Vec3 farHalfVOffset = upKBasis * farHalfVSide;
	const Vec3 farHalfHOffset = leftJBasis * farHalfHSide;

	const Vec3 nearBL = m_position + nearPlaneForward - nearHalfHOffset - nearHalfVOffset;
	const Vec3 nearBR = m_position + nearPlaneForward + nearHalfHOffset - nearHalfVOffset;
	const Vec3 nearTR = m_position + nearPlaneForward + nearHalfHOffset + nearHalfVOffset;
	const Vec3 nearTL = m_position + nearPlaneForward - nearHalfHOffset + nearHalfVOffset;

	const Vec3 farBL = m_position + farPlaneForward - farHalfHOffset - farHalfVOffset;
	const Vec3 farBR = m_position + farPlaneForward + farHalfHOffset - farHalfVOffset;
	const Vec3 farTR = m_position + farPlaneForward + farHalfHOffset + farHalfVOffset;
	const Vec3 farTL = m_position + farPlaneForward - farHalfHOffset + farHalfVOffset;

	// Major
	{
		std::vector<Vertex_PCU> majorVerts;
		majorVerts.reserve(3 * 8);

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearBL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearBR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearBR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearTR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearTR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearTL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearTL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(nearBL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));


		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farBL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farBR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farBR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farTR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farTR, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farTL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		majorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farTL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		majorVerts.push_back(Vertex_PCU(farBL, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		DebugAddWorldWireTriangleListNoneCull(majorVerts, 0.f, MAJOR_COLOR, MAJOR_COLOR);
	}


	// Minor
	{
		std::vector<Vertex_PCU> minorVerts;
		minorVerts.reserve(3 * 4 * (NUM_SLICES - 2));

		for (int i = 1; i <= NUM_SLICES - 2; ++i)
		{
			float tStart = static_cast<float>(i) / static_cast<float>(NUM_SLICES);
			float tEnd = static_cast<float>(i + 1) / static_cast<float>(NUM_SLICES);

			Vec3 bottomStart = Interpolate(farBL, farBR, tStart);
			Vec3 bottomEnd = Interpolate(farBL, farBR, tEnd);

			Vec3 rightStart = Interpolate(farBR, farTR, tStart);
			Vec3 rightEnd = Interpolate(farBR, farTR, tEnd);

			Vec3 topStart = Interpolate(farTR, farTL, tStart);
			Vec3 topEnd = Interpolate(farTR, farTL, tEnd);

			Vec3 leftStart = Interpolate(farTL, farBL, tStart);
			Vec3 leftEnd = Interpolate(farTL, farBL, tEnd);

			minorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(bottomStart, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(bottomEnd, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

			minorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(rightStart, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(rightEnd, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

			minorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(topStart, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(topEnd, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

			minorVerts.push_back(Vertex_PCU(m_position, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(leftStart, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
			minorVerts.push_back(Vertex_PCU(leftEnd, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		}

		DebugAddWorldWireTriangleListNoneCull(minorVerts, 0.f, MINOR_COLOR, MINOR_COLOR);
	}
}

void Camera::DebugDrawOrthographicFrustum() const
{
	constexpr int NUM_SLICES = 25;
	const Rgba8 COLOR(0, 255, 0);

	//const float halfWidth = (m_orthographicTopRight.x - m_orthographicBottomLeft.x) * 0.5f;
	//const float halfHeight = (m_orthographicTopRight.y - m_orthographicBottomLeft.y) * 0.5f;
	const float nearDist = m_orthographicNear;
	const float farDist = m_orthographicFar;

	const Vec2 TR = m_orthographicTopRight;
	const Vec2 BL = m_orthographicBottomLeft;

	// Camera Space
	// Near plane corners
	Vec3 nearBL(nearDist, -BL.x, BL.y); 
	Vec3 nearBR(nearDist, -TR.x, BL.y);
	Vec3 nearTR(nearDist, -TR.x, TR.y);
	Vec3 nearTL(nearDist, -BL.x, TR.y);

	// Far plane corners
	Vec3 farBL(farDist, -BL.x, BL.y);
	Vec3 farBR(farDist, -TR.x, BL.y);
	Vec3 farTR(farDist, -TR.x, TR.y);
	Vec3 farTL(farDist, -BL.x, TR.y);

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);

	Mat44 cameraToWorld;
	cameraToWorld.SetIJKT3D(forwardIBasis, leftJBasis, upKBasis, m_position);

	std::vector<Vertex_PCU> verts;
	verts.reserve(3 * 2 * 4 * NUM_SLICES);

	for (int i = 0; i < NUM_SLICES; ++i)
	{
		float t0 = static_cast<float>(i) / static_cast<float>(NUM_SLICES);
		float t1 = static_cast<float>(i + 1) / static_cast<float>(NUM_SLICES);

		// Bottom face
		Vec3 bottom_near0 = Interpolate(nearBL, nearBR, t0);
		Vec3 bottom_near1 = Interpolate(nearBL, nearBR, t1);
		Vec3 bottom_far0 = Interpolate(farBL, farBR, t0);
		Vec3 bottom_far1 = Interpolate(farBL, farBR, t1);

		verts.push_back(Vertex_PCU(bottom_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(bottom_far0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(bottom_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		verts.push_back(Vertex_PCU(bottom_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(bottom_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(bottom_near1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		// Right face
		Vec3 right_near0 = Interpolate(nearBR, nearTR, t0);
		Vec3 right_near1 = Interpolate(nearBR, nearTR, t1);
		Vec3 right_far0 = Interpolate(farBR, farTR, t0);
		Vec3 right_far1 = Interpolate(farBR, farTR, t1);

		verts.push_back(Vertex_PCU(right_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(right_far0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(right_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		verts.push_back(Vertex_PCU(right_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(right_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(right_near1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		// Top face
		Vec3 top_near0 = Interpolate(nearTR, nearTL, t0);
		Vec3 top_near1 = Interpolate(nearTR, nearTL, t1);
		Vec3 top_far0 = Interpolate(farTR, farTL, t0);
		Vec3 top_far1 = Interpolate(farTR, farTL, t1);

		verts.push_back(Vertex_PCU(top_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(top_far0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(top_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		verts.push_back(Vertex_PCU(top_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(top_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(top_near1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		// Left face
		Vec3 left_near0 = Interpolate(nearTL, nearBL, t0);
		Vec3 left_near1 = Interpolate(nearTL, nearBL, t1);
		Vec3 left_far0 = Interpolate(farTL, farBL, t0);
		Vec3 left_far1 = Interpolate(farTL, farBL, t1);

		verts.push_back(Vertex_PCU(left_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(left_far0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(left_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));

		verts.push_back(Vertex_PCU(left_near0, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(left_far1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
		verts.push_back(Vertex_PCU(left_near1, Rgba8::OPAQUE_WHITE, Vec2::ZERO));
	}

	TransformVertexArray3D(verts, cameraToWorld);

	DebugAddWorldWireTriangleListNoneCull(verts, 0.f, COLOR, COLOR);
}

