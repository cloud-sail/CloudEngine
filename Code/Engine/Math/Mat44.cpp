#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Quat.hpp"
#include "Engine/Core/EngineCommon.hpp"


//-----------------------------------------------------------------------------------------------
const Mat44 Mat44::DIRECTX_C2R(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());

//-----------------------------------------------------------------------------------------------
Mat44::Mat44()
{
	m_values[Ix] = 1.0f;
	m_values[Jy] = 1.0f;
	m_values[Kz] = 1.0f;
	m_values[Tw] = 1.0f;
}

Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;

	m_values[Kz] = 1.0f;
	m_values[Tw] = 1.0f;
}

Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;

	m_values[Tw] = 1.0f;
}

Mat44::Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

Mat44::Mat44(float const* sixteenValueBasisMajor)
{
	for (int i = 0; i < 16; ++i)
	{
		m_values[i] = sixteenValueBasisMajor[i];
	}
}

Mat44 const Mat44::MakeTranslation2D(Vec2 const& translationXY)
{
	Mat44 result = Mat44();
	result.m_values[Tx] = translationXY.x;
	result.m_values[Ty] = translationXY.y;
	return result;
}

Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 result = Mat44();
	result.m_values[Tx] = translationXYZ.x;
	result.m_values[Ty] = translationXYZ.y;
	result.m_values[Tz] = translationXYZ.z;
	return result;
}

Mat44 const Mat44::MakeUniformScale2D(float uniformScaleXY)
{
	Mat44 result = Mat44();
	result.m_values[Ix] = uniformScaleXY;
	result.m_values[Jy] = uniformScaleXY;
	return result;
}

Mat44 const Mat44::MakeUniformScale3D(float uniformScaleXYZ)
{
	Mat44 result = Mat44();
	result.m_values[Ix] = uniformScaleXYZ;
	result.m_values[Jy] = uniformScaleXYZ;
	result.m_values[Kz] = uniformScaleXYZ;
	return result;
}

Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 result = Mat44();
	result.m_values[Ix] = nonUniformScaleXY.x;
	result.m_values[Jy] = nonUniformScaleXY.y;
	return result;
}

Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 result = Mat44();
	result.m_values[Ix] = nonUniformScaleXYZ.x;
	result.m_values[Jy] = nonUniformScaleXYZ.y;
	result.m_values[Kz] = nonUniformScaleXYZ.z;
	return result;
}

Mat44 const Mat44::MakeZRotationDegrees(float rotationDegreesAboutZ)
{
	Mat44 result = Mat44();
	result.m_values[Ix] = CosDegrees(rotationDegreesAboutZ);
	result.m_values[Iy] = SinDegrees(rotationDegreesAboutZ);
	result.m_values[Jx] = -SinDegrees(rotationDegreesAboutZ);
	result.m_values[Jy] = CosDegrees(rotationDegreesAboutZ);
	return result;
}

Mat44 const Mat44::MakeYRotationDegrees(float rotationDegreesAboutY)
{
	Mat44 result = Mat44();
	result.m_values[Kz] = CosDegrees(rotationDegreesAboutY);
	result.m_values[Kx] = SinDegrees(rotationDegreesAboutY);
	result.m_values[Iz] = -SinDegrees(rotationDegreesAboutY);
	result.m_values[Ix] = CosDegrees(rotationDegreesAboutY);
	return result;
}

Mat44 const Mat44::MakeXRotationDegrees(float rotationDegreesAboutX)
{
	Mat44 result = Mat44();
	result.m_values[Jy] = CosDegrees(rotationDegreesAboutX);
	result.m_values[Jz] = SinDegrees(rotationDegreesAboutX);
	result.m_values[Ky] = -SinDegrees(rotationDegreesAboutX);
	result.m_values[Kz] = CosDegrees(rotationDegreesAboutX);
	return result;
}

Mat44 const Mat44::MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
	Vec3 invDimension(1.f / (right - left), 1.f / (top - bottom), 1.f / (zFar - zNear));

	Mat44 result;

	result.m_values[Ix] = 2.f * invDimension.x;
	result.m_values[Jy] = 2.f * invDimension.y;
	result.m_values[Kz] = invDimension.z;
	result.m_values[Tx] = -(left + right) * invDimension.x;
	result.m_values[Ty] = -(bottom + top) * invDimension.y;
	result.m_values[Tz] = -zNear * invDimension.z;

	return result;
	//return Mat44(Vec3(2.f * invDimension.x, 0.f, 0.f),
	//	Vec3(0.f, 2.f * invDimension.y, 0.f),
	//	Vec3(0.f, 0.f, invDimension.z),
	//	Vec3(-(left + right) * invDimension.x, -(bottom + top) * invDimension.y, -0.5f * (zNear + zFar) * invDimension.z + 0.5f));
}

Mat44 const Mat44::MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
	Mat44 result;

	float halfFovY = fovYDegrees * 0.5f;
	float cotHalfFov = CosDegrees(halfFovY) / SinDegrees(halfFovY);
	float valueKz = zFar / (zFar - zNear);


	result.m_values[Ix] = cotHalfFov / aspect;
	result.m_values[Jy] = cotHalfFov;
	result.m_values[Kz] = valueKz;
	result.m_values[Tw] = 0.f;
	result.m_values[Tz] = - zNear * valueKz;
	result.m_values[Kw] = 1.f;

	return result;

	// when zNear = 0.f, transformation is broken
	//float top = zNear * TanDegrees(fovYDegrees * 0.5f);
	//float bottom = -top;
	//float right = top * aspect;
	//float left = -right;

	//Mat44 matPerspToOrtho(
	//	Vec4(zNear, 0.f, 0.f, 0.f),
	//	Vec4(0.f, zNear, 0.f, 0.f),
	//	Vec4(0.f, 0.f, zNear + zFar, 1.f),
	//	Vec4(0.f, 0.f, -zNear * zFar, 0.f));

	//Mat44 result = MakeOrthoProjection(left, right, bottom, top, zNear, zFar);
	//result.Append(matPerspToOrtho);

	//return result;
}

STATIC Mat44 const Mat44::MakeFromX(Vec3 const& xAxis)
{
	// Try Best make Z.z more positive
	Vec3 iBasis = xAxis.GetNormalized();

	Vec3 upDirection = (fabsf(iBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;

	Vec3 jBasis = CrossProduct3D(upDirection, iBasis).GetNormalized();
	Vec3 kBasis = CrossProduct3D(iBasis, jBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}



STATIC Mat44 const Mat44::MakeFromY(Vec3 const& yAxis)
{
	// Try Best make X.z more positive
	Vec3 jBasis = yAxis.GetNormalized();

	Vec3 upDirection = (fabsf(jBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;

	Vec3 kBasis = CrossProduct3D(upDirection, jBasis).GetNormalized();
	Vec3 iBasis = CrossProduct3D(jBasis, kBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromZ(Vec3 const& zAxis)
{
	// Try Best make Y.z more positive
	Vec3 kBasis = zAxis.GetNormalized();

	Vec3 upDirection = (fabsf(kBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;

	Vec3 iBasis = CrossProduct3D(upDirection, kBasis).GetNormalized();
	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromXY(Vec3 const& xAxis, Vec3 const& yAxis)
{
	Vec3 iBasis = xAxis.GetNormalized();
	Vec3 secondaryAxis = yAxis.GetNormalized();

	if (fabsf(DotProduct3D(iBasis, secondaryAxis)) == 1.f)
	{
		secondaryAxis = (fabsf(iBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
	}

	Vec3 kBasis = CrossProduct3D(iBasis, secondaryAxis).GetNormalized();
	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromXZ(Vec3 const& xAxis, Vec3 const& zAxis)
{
	Vec3 iBasis = xAxis.GetNormalized();
	Vec3 secondaryAxis = zAxis.GetNormalized();
	
	// Edge Case: Primary and secondary axis directions are paralleled, 
	// then we need to pick a default direction for secondary axis directions
	// And if first & secondary & default direction are all paralleled, choose another default direction.
	// Not sure when to use nearly equals
	if (fabsf(DotProduct3D(iBasis, secondaryAxis)) == 1.f)
	{
		secondaryAxis = (fabsf(iBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
	}

	Vec3 jBasis = CrossProduct3D(secondaryAxis, iBasis).GetNormalized();
	Vec3 kBasis = CrossProduct3D(iBasis, jBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}


STATIC Mat44 const Mat44::MakeFromYX(Vec3 const& yAxis, Vec3 const& xAxis)
{
	Vec3 jBasis = yAxis.GetNormalized();
	Vec3 secondaryAxis = xAxis.GetNormalized();

	if (fabsf(DotProduct3D(jBasis, secondaryAxis)) == 1.f)
	{
		secondaryAxis = (fabsf(jBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
	}

	Vec3 kBasis = CrossProduct3D(secondaryAxis, jBasis).GetNormalized();
	Vec3 iBasis = CrossProduct3D(jBasis, kBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromYZ(Vec3 const& yAxis, Vec3 const& zAxis)
{
	Vec3 jBasis = yAxis.GetNormalized();
	Vec3 secondaryAxis = zAxis.GetNormalized();

	if (fabsf(DotProduct3D(jBasis, secondaryAxis)) == 1.f)
	{
		secondaryAxis = (fabsf(jBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
	}

	Vec3 iBasis = CrossProduct3D(jBasis, secondaryAxis).GetNormalized();
	Vec3 kBasis = CrossProduct3D(iBasis, jBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromZX(Vec3 const& zAxis, Vec3 const& xAxis)
{
 	Vec3 kBasis = zAxis.GetNormalized();
 	Vec3 secondaryAxis = xAxis.GetNormalized();
 
 	if (fabsf(DotProduct3D(kBasis, secondaryAxis)) == 1.f)
 	{
 		secondaryAxis = (fabsf(kBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
 	}
 
 	Vec3 jBasis = CrossProduct3D(kBasis, secondaryAxis).GetNormalized();
 	Vec3 iBasis = CrossProduct3D(jBasis, kBasis);
 
 	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromZY(Vec3 const& zAxis, Vec3 const& yAxis)
{
	Vec3 kBasis = zAxis.GetNormalized();
	Vec3 secondaryAxis = yAxis.GetNormalized();

	if (fabsf(DotProduct3D(kBasis, secondaryAxis)) == 1.f)
	{
		secondaryAxis = (fabsf(kBasis.z) < 1.f) ? Vec3::ZAXIS : Vec3::XAXIS;
	}

	Vec3 iBasis = CrossProduct3D(secondaryAxis, kBasis).GetNormalized();
	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

STATIC Mat44 const Mat44::MakeFromUnitQuat(Quat q)
{
	// Assume Quat is unit quaternion
	// No check here
	Mat44 result;

	float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
	float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
	float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
	float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;

	result.m_values[Ix] = 1.f - (yy + zz);	result.m_values[Jx] = xy - wz;			result.m_values[Kx] = xz + wy;			result.m_values[Tx] = 0.f;
	result.m_values[Iy] = xy + wz;			result.m_values[Jy] = 1.f - (xx + zz);	result.m_values[Ky] = yz - wx;			result.m_values[Ty] = 0.f;
	result.m_values[Iz] = xz - wy;			result.m_values[Jz] = yz + wx;			result.m_values[Kz] = 1.f - (xx + yy);	result.m_values[Tz] = 0.f;
	result.m_values[Iw] = 0.f;				result.m_values[Jw] = 0.f;				result.m_values[Kw] = 0.f;				result.m_values[Tw] = 1.f;

	return result;
}

STATIC Mat44 const Mat44::MakeFromNonUnitQuat(Quat q)
{
	// rotate forward, left, up by q. not normalized. result will be weird
	Vec3 iBasis = q.RotateVector(Vec3::FORWARD);
	Vec3 jBasis = q.RotateVector(Vec3::LEFT);
	Vec3 kBasis = q.RotateVector(Vec3::UP);

	return Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
}

//-----------------------------------------------------------------------------------------------
Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
	Vec2 result;
	result.x = m_values[Ix] * vectorQuantityXY.x + m_values[Jx] * vectorQuantityXY.y;
	result.y = m_values[Iy] * vectorQuantityXY.x + m_values[Jy] * vectorQuantityXY.y;
	return result;
}

Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
	Vec3 result;
	result.x = m_values[Ix] * vectorQuantityXYZ.x + m_values[Jx] * vectorQuantityXYZ.y + m_values[Kx] * vectorQuantityXYZ.z;
	result.y = m_values[Iy] * vectorQuantityXYZ.x + m_values[Jy] * vectorQuantityXYZ.y + m_values[Ky] * vectorQuantityXYZ.z;
	result.z = m_values[Iz] * vectorQuantityXYZ.x + m_values[Jz] * vectorQuantityXYZ.y + m_values[Kz] * vectorQuantityXYZ.z;
	return result;
}

Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
	Vec2 result;
	result.x = m_values[Ix] * positionXY.x + m_values[Jx] * positionXY.y + m_values[Tx];
	result.y = m_values[Iy] * positionXY.x + m_values[Jy] * positionXY.y + m_values[Ty];
	return result;
}

Vec3 const Mat44::TransformPosition3D(Vec3 const& position3D) const
{
	Vec3 result;
	result.x = m_values[Ix] * position3D.x + m_values[Jx] * position3D.y + m_values[Kx] * position3D.z + m_values[Tx];
	result.y = m_values[Iy] * position3D.x + m_values[Jy] * position3D.y + m_values[Ky] * position3D.z + m_values[Ty];
	result.z = m_values[Iz] * position3D.x + m_values[Jz] * position3D.y + m_values[Kz] * position3D.z + m_values[Tz];
	return result;
}

Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogeneourPoint3D) const
{
	Vec4 result;
	result.x = m_values[Ix] * homogeneourPoint3D.x + m_values[Jx] * homogeneourPoint3D.y + m_values[Kx] * homogeneourPoint3D.z + m_values[Tx] * homogeneourPoint3D.w;
	result.y = m_values[Iy] * homogeneourPoint3D.x + m_values[Jy] * homogeneourPoint3D.y + m_values[Ky] * homogeneourPoint3D.z + m_values[Ty] * homogeneourPoint3D.w;
	result.z = m_values[Iz] * homogeneourPoint3D.x + m_values[Jz] * homogeneourPoint3D.y + m_values[Kz] * homogeneourPoint3D.z + m_values[Tz] * homogeneourPoint3D.w;
	result.w = m_values[Iw] * homogeneourPoint3D.x + m_values[Jw] * homogeneourPoint3D.y + m_values[Kw] * homogeneourPoint3D.z + m_values[Tw] * homogeneourPoint3D.w;
	return result;
}

float* Mat44::GetAsFloatArray()
{
	return m_values;
}

float const* Mat44::GetAsFloatArray() const
{
	return m_values;
}

Vec2 const Mat44::GetIBasis2D() const
{
	Vec2 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];

	return result;
}

Vec2 const Mat44::GetJBasis2D() const
{
	Vec2 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];

	return result;
}

Vec2 const Mat44::GetTranslation2D() const
{
	Vec2 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];

	return result;
}

Vec3 const Mat44::GetIBasis3D() const
{
	Vec3 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];
	result.z = m_values[Iz];

	return result;
}

Vec3 const Mat44::GetJBasis3D() const
{
	Vec3 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];
	result.z = m_values[Jz];

	return result;
}

Vec3 const Mat44::GetKBasis3D() const
{
	Vec3 result;
	result.x = m_values[Kx];
	result.y = m_values[Ky];
	result.z = m_values[Kz];

	return result;
}

Vec3 const Mat44::GetTranslation3D() const
{
	Vec3 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];
	result.z = m_values[Tz];

	return result;
}

Vec4 const Mat44::GetIBasis4D() const
{
	Vec4 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];
	result.z = m_values[Iz];
	result.w = m_values[Iw];

	return result;
}

Vec4 const Mat44::GetJBasis4D() const
{
	Vec4 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];
	result.z = m_values[Jz];
	result.w = m_values[Jw];

	return result;
}

Vec4 const Mat44::GetKBasis4D() const
{
	Vec4 result;
	result.x = m_values[Kx];
	result.y = m_values[Ky];
	result.z = m_values[Kz];
	result.w = m_values[Kw];

	return result;;
}

Vec4 const Mat44::GetTranslation4D() const
{
	Vec4 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];
	result.z = m_values[Tz];
	result.w = m_values[Tw];

	return result;
}

Mat44 const Mat44::GetOrthonormalInverse() const
{
	// *this = T * R
	// inv(*this) = R.Transpose() * inv(T)
	Vec3 iBasis = GetIBasis3D();
	Vec3 jBasis = GetJBasis3D();
	Vec3 kBasis = GetKBasis3D();
	Vec3 translation = GetTranslation3D();

	Mat44 result(GetIBasis3D(), GetJBasis3D(), GetKBasis3D(), Vec3::ZERO);
	result.TransposeIJK();

	Vec3 newTranslation(-DotProduct3D(translation, iBasis), -DotProduct3D(translation, jBasis), -DotProduct3D(translation, kBasis));
	result.SetTranslation3D(newTranslation);

	return result;
}

EulerAngles const Mat44::GetEulerAngles() const
{
	// not sure the transform matrix is orthonormal
	Vec3 iBasis = GetIBasis3D().GetNormalized();
	Vec3 jBasis = GetJBasis3D().GetNormalized();
	Vec3 kBasis = GetKBasis3D().GetNormalized();

	float yawDegrees = Atan2Degrees(iBasis.y, iBasis.x);
	//float pitchDegrees = Atan2Degrees(iBasis.z, sqrtf(iBasis.x * iBasis.x + iBasis.y * iBasis.y));
	float pitchDegrees = ConvertRadiansToDegrees(- asinf(iBasis.z));

	// Get jBasis of EulerAngles(yawDegrees, pitchDegrees, 0.f)
	//Vec3 tempJBasis = EulerAngles(yawDegrees, pitchDegrees, 0.f).GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D();
	Vec3 tempJBasis = Vec3(-SinDegrees(yawDegrees), CosDegrees(yawDegrees), 0.f);
	float rollDegrees = Atan2Degrees(-DotProduct3D(kBasis, tempJBasis), DotProduct3D(jBasis, tempJBasis));

	return EulerAngles(yawDegrees, pitchDegrees, rollDegrees);
}

Quat const Mat44::GetQuat() const
{
	// Ken Shoemake Quaternions
	// Need to be orthonormal
	Quat q;

	//float const tr = m_values[Ix] + m_values[Jy] + m_values[Kz];
	//float s;

	float fourWSquaredMinus1 = m_values[Ix] + m_values[Jy] + m_values[Kz];
	float fourXSquaredMinus1 = m_values[Ix] - m_values[Jy] - m_values[Kz];
	float fourYSquaredMinus1 = m_values[Jy] - m_values[Ix] - m_values[Kz];
	float fourZSquaredMinus1 = m_values[Kz] - m_values[Ix] - m_values[Jy];

	int biggestIndex = 0;
	float fourBiggestSquaredMinus1 = fourWSquaredMinus1;

	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}

	float biggestVal = sqrtf(fourBiggestSquaredMinus1 + 1) * 0.5f;
	float mult = 0.25f / biggestVal;

	switch (biggestIndex)
	{
	case 0:
		q.w = biggestVal;
		q.x = (m_values[Jz] - m_values[Ky]) * mult;
		q.y = (m_values[Kx] - m_values[Iz]) * mult;
		q.z = (m_values[Iy] - m_values[Jx]) * mult;
		break;
	case 1:
		q.x = biggestVal;
		q.w = (m_values[Jz] - m_values[Ky]) * mult;
		q.y = (m_values[Iy] + m_values[Jx]) * mult;
		q.z = (m_values[Iz] + m_values[Kx]) * mult;
		break;
	case 2:
		q.y = biggestVal;
		q.w = (m_values[Kx] - m_values[Iz]) * mult;
		q.x = (m_values[Iy] + m_values[Jx]) * mult;
		q.z = (m_values[Jz] + m_values[Ky]) * mult;
		break;
	case 3:
		q.z = biggestVal;
		q.w = (m_values[Iy] - m_values[Jx]) * mult;
		q.x = (m_values[Iz] + m_values[Kx]) * mult;
		q.y = (m_values[Jz] + m_values[Ky]) * mult;
		break;
	}


	/*
	result.m_values[Ix] = 1.f - (yy + zz);	result.m_values[Jx] = xy - wz;			result.m_values[Kx] = xz + wy;			result.m_values[Tx] = 0.f;
	result.m_values[Iy] = xy + wz;			result.m_values[Jy] = 1.f - (xx + zz);	result.m_values[Ky] = yz - wx;			result.m_values[Ty] = 0.f;
	result.m_values[Iz] = xz - wy;			result.m_values[Jz] = yz + wx;			result.m_values[Kz] = 1.f - (xx + yy);	result.m_values[Tz] = 0.f;
	result.m_values[Iw] = 0.f;				result.m_values[Jw] = 0.f;				result.m_values[Kw] = 0.f;				result.m_values[Tw] = 1.f;
	*/

	return q;
}

void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;
}

void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;
}

void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

void Mat44::Transpose()
{
	Mat44 oldMe = *this;
	float const* old = oldMe.GetAsFloatArray();

	m_values[Iy] = old[Jx];
	m_values[Iz] = old[Kx];
	m_values[Iw] = old[Tx];
	
	m_values[Jx] = old[Iy];
	m_values[Jz] = old[Ky];
	m_values[Jw] = old[Ty];
				
	m_values[Kx] = old[Iz];
	m_values[Ky] = old[Jz];
	m_values[Kw] = old[Tz];
		
	m_values[Tx] = old[Iw];
	m_values[Ty] = old[Jw];
	m_values[Tz] = old[Kw];
}

void Mat44::TransposeIJK()
{
	Mat44 oldMe = *this;
	float const* old = oldMe.GetAsFloatArray();

	m_values[Iy] = old[Jx];
	m_values[Iz] = old[Kx];

	m_values[Jx] = old[Iy];
	m_values[Jz] = old[Ky];

	m_values[Kx] = old[Iz];
	m_values[Ky] = old[Jz];
}

void Mat44::Orthonormalize_IFwd_JLeft_KUp()
{
	Vec3 iBasis = GetIBasis3D();
	Vec3 jBasis = GetJBasis3D();
	Vec3 kBasis = GetKBasis3D();

	iBasis = iBasis.GetNormalized();

	kBasis = kBasis - DotProduct3D(iBasis, kBasis) * iBasis;
	kBasis = kBasis.GetNormalized();
	
	jBasis = jBasis - DotProduct3D(iBasis, jBasis) * iBasis - DotProduct3D(kBasis, jBasis) * kBasis;
	jBasis = jBasis.GetNormalized();

	SetIJK3D(iBasis, jBasis, kBasis);
}

void Mat44::Append(Mat44 const& appendThis)
{
	Mat44 oldMe = *this;
	float const* old = oldMe.GetAsFloatArray();
	float const* append = appendThis.GetAsFloatArray();
	m_values[Ix] = old[Ix] * append[Ix] + old[Jx] * append[Iy] + old[Kx] * append[Iz] + old[Tx] * append[Iw];
	m_values[Iy] = old[Iy] * append[Ix] + old[Jy] * append[Iy] + old[Ky] * append[Iz] + old[Ty] * append[Iw];
	m_values[Iz] = old[Iz] * append[Ix] + old[Jz] * append[Iy] + old[Kz] * append[Iz] + old[Tz] * append[Iw];
	m_values[Iw] = old[Iw] * append[Ix] + old[Jw] * append[Iy] + old[Kw] * append[Iz] + old[Tw] * append[Iw];

	m_values[Jx] = old[Ix] * append[Jx] + old[Jx] * append[Jy] + old[Kx] * append[Jz] + old[Tx] * append[Jw];
	m_values[Jy] = old[Iy] * append[Jx] + old[Jy] * append[Jy] + old[Ky] * append[Jz] + old[Ty] * append[Jw];
	m_values[Jz] = old[Iz] * append[Jx] + old[Jz] * append[Jy] + old[Kz] * append[Jz] + old[Tz] * append[Jw];
	m_values[Jw] = old[Iw] * append[Jx] + old[Jw] * append[Jy] + old[Kw] * append[Jz] + old[Tw] * append[Jw];

	m_values[Kx] = old[Ix] * append[Kx] + old[Jx] * append[Ky] + old[Kx] * append[Kz] + old[Tx] * append[Kw];
	m_values[Ky] = old[Iy] * append[Kx] + old[Jy] * append[Ky] + old[Ky] * append[Kz] + old[Ty] * append[Kw];
	m_values[Kz] = old[Iz] * append[Kx] + old[Jz] * append[Ky] + old[Kz] * append[Kz] + old[Tz] * append[Kw];
	m_values[Kw] = old[Iw] * append[Kx] + old[Jw] * append[Ky] + old[Kw] * append[Kz] + old[Tw] * append[Kw];

	m_values[Tx] = old[Ix] * append[Tx] + old[Jx] * append[Ty] + old[Kx] * append[Tz] + old[Tx] * append[Tw];
	m_values[Ty] = old[Iy] * append[Tx] + old[Jy] * append[Ty] + old[Ky] * append[Tz] + old[Ty] * append[Tw];
	m_values[Tz] = old[Iz] * append[Tx] + old[Jz] * append[Ty] + old[Kz] * append[Tz] + old[Tz] * append[Tw];
	m_values[Tw] = old[Iw] * append[Tx] + old[Jw] * append[Ty] + old[Kw] * append[Tz] + old[Tw] * append[Tw];
}

void Mat44::AppendZRotation(float degreesRotationAboutZ)
{
	Append(MakeZRotationDegrees(degreesRotationAboutZ));
}

void Mat44::AppendYRotation(float degreesRotationAboutY)
{
	Append(MakeYRotationDegrees(degreesRotationAboutY));
}

void Mat44::AppendXRotation(float degreesRotationAboutX)
{
	Append(MakeXRotationDegrees(degreesRotationAboutX));
}

void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
	Append(MakeTranslation2D(translationXY));
}

void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
	Append(MakeTranslation3D(translationXYZ));
}

void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{
	m_values[Ix] *= uniformScaleXY;
	m_values[Iy] *= uniformScaleXY;
	m_values[Iz] *= uniformScaleXY;
	m_values[Iw] *= uniformScaleXY;

	m_values[Jx] *= uniformScaleXY;
	m_values[Jy] *= uniformScaleXY;
	m_values[Jz] *= uniformScaleXY;
	m_values[Jw] *= uniformScaleXY;
}

void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{
	m_values[Ix] *= uniformScaleXYZ;
	m_values[Iy] *= uniformScaleXYZ;
	m_values[Iz] *= uniformScaleXYZ;
	m_values[Iw] *= uniformScaleXYZ;

	m_values[Jx] *= uniformScaleXYZ;
	m_values[Jy] *= uniformScaleXYZ;
	m_values[Jz] *= uniformScaleXYZ;
	m_values[Jw] *= uniformScaleXYZ;

	m_values[Kx] *= uniformScaleXYZ;
	m_values[Ky] *= uniformScaleXYZ;
	m_values[Kz] *= uniformScaleXYZ;
	m_values[Kw] *= uniformScaleXYZ;
}

void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
	m_values[Ix] *= nonUniformScaleXY.x;
	m_values[Iy] *= nonUniformScaleXY.x;
	m_values[Iz] *= nonUniformScaleXY.x;
	m_values[Iw] *= nonUniformScaleXY.x;

	m_values[Jx] *= nonUniformScaleXY.y;
	m_values[Jy] *= nonUniformScaleXY.y;
	m_values[Jz] *= nonUniformScaleXY.y;
	m_values[Jw] *= nonUniformScaleXY.y;
}

void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
	m_values[Ix] *= nonUniformScaleXYZ.x;
	m_values[Iy] *= nonUniformScaleXYZ.x;
	m_values[Iz] *= nonUniformScaleXYZ.x;
	m_values[Iw] *= nonUniformScaleXYZ.x;

	m_values[Jx] *= nonUniformScaleXYZ.y;
	m_values[Jy] *= nonUniformScaleXYZ.y;
	m_values[Jz] *= nonUniformScaleXYZ.y;
	m_values[Jw] *= nonUniformScaleXYZ.y;

	m_values[Kx] *= nonUniformScaleXYZ.z;
	m_values[Ky] *= nonUniformScaleXYZ.z;
	m_values[Kz] *= nonUniformScaleXYZ.z;
	m_values[Kw] *= nonUniformScaleXYZ.z;
}



