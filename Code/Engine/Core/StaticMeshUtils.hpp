#pragma once

#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <vector>
#include <string>


struct StaticModelInfo
{
	std::string m_modelFilePath = "";
	std::string m_shaderName = "";
	std::string m_diffuseMapFilePath = "";
	std::string m_normalMapFilePath = "";
	std::string m_specGlossEmitMapFilePath = "";
	float		m_unitsPerMeter = 1.f;
	std::string m_xDirection = "forward";
	std::string m_yDirection = "left";
	std::string m_zDirection = "up";
	bool		m_frontCCW = true;
	Vec3		m_translation = Vec3::ZERO;
};


namespace StaticMeshUtils
{

}

bool LoadOBJFromXML(std::vector<Vertex_PCUTBN>& out_verts, StaticModelInfo& out_modelInfo, const char* modelXmlFilePath);
bool LoadOBJFromXML(std::vector<Vertex_PCUTBN>& out_verts, const char* modelXmlFilePath);
bool ParseOBJMeshTextBuffer(std::vector<Vertex_PCUTBN>& out_verts, std::string const& fileString, bool isForwardCCW);


