#pragma once

#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <vector>
#include <string>


namespace StaticMeshUtils
{

	//bool Load


	//bool ParseOBJMeshTextBuffer(std::vector<Vertex_PCUTBN> a)
	//{
	//	// ignore mtllib
	//}



}

bool LoadOBJFromXML(std::vector<Vertex_PCUTBN>& out_verts, const char* modelXmlFilePath);
bool ParseOBJMeshTextBuffer(std::vector<Vertex_PCUTBN>& out_verts, std::string const& fileString, bool isForwardCCW);


