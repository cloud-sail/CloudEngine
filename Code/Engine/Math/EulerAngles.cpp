#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"


EulerAngles::EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees)
	: m_yawDegrees(yawDegrees)
	, m_pitchDegrees(pitchDegrees)
	, m_rollDegrees(rollDegrees)
{
}

void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const
{
	Mat44 rotationMat = GetAsMatrix_IFwd_JLeft_KUp();
	out_forwardIBasis = rotationMat.GetIBasis3D();
	out_leftJBasis = rotationMat.GetJBasis3D();
	out_upKBasis = rotationMat.GetKBasis3D();
}

Mat44 EulerAngles::GetAsMatrix_IFwd_JLeft_KUp() const
{
	//Mat44 result;
	//result.AppendZRotation(m_yawDegrees);
	//result.AppendYRotation(m_pitchDegrees);
	//result.AppendXRotation(m_rollDegrees);
	//return result;
	float cYaw = CosDegrees(m_yawDegrees);
	float sYaw = SinDegrees(m_yawDegrees);
	float cPitch = CosDegrees(m_pitchDegrees);
	float sPitch = SinDegrees(m_pitchDegrees);
	float cRoll = CosDegrees(m_rollDegrees);
	float sRoll = SinDegrees(m_rollDegrees);

	Mat44 result(Vec3(cYaw * cPitch, sYaw * cPitch, -sPitch),
		Vec3(cYaw * sPitch * sRoll - sYaw * cRoll, sYaw * sPitch * sRoll + cYaw * cRoll, cPitch * sRoll),
		Vec3(cYaw * sPitch * cRoll + sYaw * sRoll, sYaw * sPitch * cRoll - cYaw * sRoll, cPitch * cRoll),
		Vec3::ZERO);

	return result;
}

void EulerAngles::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, ',');
	if (tokens.size() != 3)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for EulerAngles! text: %s", text));
	}
	m_yawDegrees = static_cast<float>(atof(tokens[0].c_str()));
	m_pitchDegrees = static_cast<float>(atof(tokens[1].c_str()));
	m_rollDegrees = static_cast<float>(atof(tokens[1].c_str()));
}
