#pragma once
//-----------------------------------------------------------------------------------------------
struct Vec2;
struct Vec3;
struct Vec4;
struct EulerAngles;
struct Quat;

//-----------------------------------------------------------------------------------------------
struct Mat44
{
	enum {	Ix, Iy, Iz, Iw, 
			Jx, Jy, Jz, Jw, 
			Kx, Ky, Kz, Kw, 
			Tx, Ty, Tz, Tw };
	float m_values[16] = {};

	static const Mat44 DIRECTX_C2R; // Camera Space to Render Space 
	// (orthonormal camera does not need this, because it is already on xy-plane.
	// and z does not need to flip if all of z are zero)


	// Constructors
	Mat44(); // Default constructor is IDENTITY matrix
	explicit Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D);
	explicit Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D);
	explicit Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D);
	explicit Mat44(float const* sixteenValueBasisMajor);

	// Static Makes
	static Mat44 const MakeTranslation2D(Vec2 const& translationXY);
	static Mat44 const MakeTranslation3D(Vec3 const& translationXYZ);
	static Mat44 const MakeUniformScale2D(float uniformScaleXY);
	static Mat44 const MakeUniformScale3D(float uniformScaleXYZ);
	static Mat44 const MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY);
	static Mat44 const MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ);
	static Mat44 const MakeZRotationDegrees(float rotationDegreesAboutZ);
	static Mat44 const MakeYRotationDegrees(float rotationDegreesAboutY);
	static Mat44 const MakeXRotationDegrees(float rotationDegreesAboutX);
	static Mat44 const MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar);
	static Mat44 const MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar);

	static Mat44 const MakeFromX(Vec3 const& xAxis);
	static Mat44 const MakeFromY(Vec3 const& yAxis);
	static Mat44 const MakeFromZ(Vec3 const& zAxis);
	static Mat44 const MakeFromXY(Vec3 const& xAxis, Vec3 const& yAxis);
	static Mat44 const MakeFromXZ(Vec3 const& xAxis, Vec3 const& zAxis);
	static Mat44 const MakeFromYX(Vec3 const& yAxis, Vec3 const& xAxis);
	static Mat44 const MakeFromYZ(Vec3 const& yAxis, Vec3 const& zAxis);
	static Mat44 const MakeFromZX(Vec3 const& zAxis, Vec3 const& xAxis);
	static Mat44 const MakeFromZY(Vec3 const& zAxis, Vec3 const& yAxis);

	static Mat44 const MakeFromUnitQuat(Quat q);
	static Mat44 const MakeFromNonUnitQuat(Quat q);

	// Transforms
	Vec2 const TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const; // assumes z=0, w=0
	Vec3 const TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const; // assumes w=0
	Vec2 const TransformPosition2D(Vec2 const& positionXY) const; // assumes z=0, w=1
	Vec3 const TransformPosition3D(Vec3 const& position3D) const; // assumes w=1
	Vec4 const TransformHomogeneous3D(Vec4 const& homogeneourPoint3D) const; // w is provided

	// Accessors
	float* GetAsFloatArray(); // non-const (mutable) version
	float const* GetAsFloatArray() const; // const version, used only when Mat44 is const
	Vec2 const GetIBasis2D() const;
	Vec2 const GetJBasis2D() const;
	Vec2 const GetTranslation2D() const;
	Vec3 const GetIBasis3D() const;
	Vec3 const GetJBasis3D() const;
	Vec3 const GetKBasis3D() const;
	Vec3 const GetTranslation3D() const;
	Vec4 const GetIBasis4D() const;
	Vec4 const GetJBasis4D() const;
	Vec4 const GetKBasis4D() const;
	Vec4 const GetTranslation4D() const;
	Mat44 const GetOrthonormalInverse() const; // Only works for orthonormal affine matrices
	EulerAngles const GetEulerAngles() const; // Only works for orthonormal matrices
	Quat const GetQuat() const;

	// Mutators
	void SetTranslation2D(Vec2 const& translationXY); // Sets translationZ = 0, translationW = 1
	void SetTranslation3D(Vec3 const& translationXYZ); // Sets translationW = 1
	void SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D); // Sets z=0, w=0 for i&j, w=1 for t; does not modify k or t
	void SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY); // Sets z=0, w=0 for ij, w=1 for t; does not modify k
	void SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D); // Sets w=0 for ijk; does not modify t
	void SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ); // Sets w=0 for ijk, w=1 for t
	void SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D); // All 16 values provided
	void Transpose(); // Swap columns with rows
	void TransposeIJK(); // Only when it is a Rotation Matrix
	void Orthonormalize_IFwd_JLeft_KUp(); // Forward is canonical, Up is secondary, Left is tertiary
	void Inverse();

	void Append(Mat44 const& appendThis); 
	void AppendZRotation(float degreesRotationAboutZ);
	void AppendYRotation(float degreesRotationAboutY);
	void AppendXRotation(float degreesRotationAboutX);
	void AppendTranslation2D(Vec2 const& translationXY);
	void AppendTranslation3D(Vec3 const& translationXYZ);
	void AppendScaleUniform2D(float uniformScaleXY);
	void AppendScaleUniform3D(float uniformScaleXYZ);
	void AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY);
	void AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ);
};

