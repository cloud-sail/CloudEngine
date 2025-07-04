#include "Engine/Core/StaticMeshUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <string>

struct FaceElement
{
	int v = -100;
	int vt = -100;
	int vn = -100;
};

struct OBJData
{
	std::vector<Vec3> vertices;		// v x y z [w]
	std::vector<Vec2> texCoords;	// vt u [v w]
	std::vector<Vec3> normals;		// vn x y z
	std::vector<std::vector<FaceElement>>  faces; // f 1/2 1/2/3 4//5 1//

};

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
};

Vec3 GetVec3FromString(std::string const& direction)
{
	if (direction == "left")
	{
		return Vec3::LEFT;
	}
	else if (direction == "right")
	{
		return Vec3::RIGHT;
	}
	else if (direction == "up")
	{
		return Vec3::UP;
	}
	else if (direction == "down")
	{
		return Vec3::DOWN;
	}
	else if (direction == "forward")
	{
		return Vec3::FORWARD;
	}
	else if (direction == "backward")
	{
		return Vec3::BACKWARD;
	}
	ERROR_AND_DIE("Wrong string for direction: must be left/right/up/down/forward/backward");
}

void OrthonormalizeTB(Vec3& tangent, Vec3& bitangent, Vec3 const& normal)
{
	tangent = (tangent - DotProduct3D(tangent, normal) * normal).GetNormalized();
	bitangent = (bitangent - DotProduct3D(bitangent, normal) * normal - DotProduct3D(bitangent, tangent) * tangent).GetNormalized();
}


//-----------------------------------------------------------------------------------------------
bool LoadOBJFromXML(std::vector<Vertex_PCUTBN>& out_verts, const char* modelXmlFilePath)
{
	XmlDocument modelXML;
	XmlResult result = modelXML.LoadFile(modelXmlFilePath);
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_RECOVERABLE(Stringf("Failed to load OBJ from XML file \"%s\"", modelXmlFilePath));
		return false;
	}
	XmlElement* rootElement = modelXML.RootElement();
	if (rootElement == nullptr)
	{
		ERROR_RECOVERABLE(Stringf("Model XML file \"%s\" was invalid (missing root element)", modelXmlFilePath));
		return false;
	}

	StaticModelInfo modelInfo;
	modelInfo.m_modelFilePath				= ParseXmlAttribute(*rootElement, "objFile", modelInfo.m_modelFilePath);
	modelInfo.m_shaderName					= ParseXmlAttribute(*rootElement, "shader", modelInfo.m_shaderName);
	modelInfo.m_diffuseMapFilePath			= ParseXmlAttribute(*rootElement, "diffuseMap", modelInfo.m_diffuseMapFilePath);
	modelInfo.m_normalMapFilePath			= ParseXmlAttribute(*rootElement, "normalMap", modelInfo.m_normalMapFilePath);
	modelInfo.m_specGlossEmitMapFilePath	= ParseXmlAttribute(*rootElement, "specGlossEmitMap", modelInfo.m_specGlossEmitMapFilePath);
	modelInfo.m_unitsPerMeter				= ParseXmlAttribute(*rootElement, "unitsPerMeter", modelInfo.m_unitsPerMeter);
	modelInfo.m_xDirection					= ParseXmlAttribute(*rootElement, "x", modelInfo.m_xDirection);
	modelInfo.m_yDirection					= ParseXmlAttribute(*rootElement, "y", modelInfo.m_yDirection);
	modelInfo.m_zDirection					= ParseXmlAttribute(*rootElement, "z", modelInfo.m_zDirection);
	modelInfo.m_frontCCW					= ParseXmlAttribute(*rootElement, "frontCounterClockwise", modelInfo.m_frontCCW);

	// NOT USED Shader Name

	std::string fileString;
	FileReadToString(fileString, modelInfo.m_modelFilePath);

	ParseOBJMeshTextBuffer(out_verts, fileString, modelInfo.m_frontCCW);



	Mat44 rotTransform = Mat44(GetVec3FromString(modelInfo.m_xDirection),
							GetVec3FromString(modelInfo.m_yDirection),
							GetVec3FromString(modelInfo.m_zDirection),
							Vec3::ZERO);
	Mat44 tbnTransform = rotTransform;
	Mat44 posTransform = rotTransform;
	posTransform.AppendScaleUniform3D(1.f / modelInfo.m_unitsPerMeter);

	TransformVertexArray3D(out_verts, posTransform, true, false);
	TransformVertexArray3D(out_verts, tbnTransform, false, true);

	return true;
}

bool ParseOBJMeshTextBuffer(std::vector<Vertex_PCUTBN>& out_verts, std::string const& fileString, bool isForwardCCW)
{
	Strings textLines = SplitStringOnDelimiter(fileString, '\n');

	OBJData data;

	size_t numLines = textLines.size();
	out_verts.reserve(numLines / 2);
	data.vertices.reserve(numLines / 4);
	data.normals.reserve(numLines / 4);
	data.texCoords.reserve(numLines / 4);
	data.faces.reserve(numLines / 2);

	for (std::string & line : textLines)
	{
		TrimSpace(line);

		if (line[0] == '#') continue; // comment line

		Strings chunks = SplitStringOnDelimiterAndDiscardEmpty(line, ' ');

		size_t numChunks = chunks.size();
		if (numChunks == 0) continue; // empty line

		if (chunks[0] == "v")
		{
			if (numChunks < 4)
			{
				ERROR_RECOVERABLE(Stringf("Not enough elements for vertex, v x y z [w]: %s", line.c_str()));
				return false;
			}
			Vec3 tempVertex = Vec3(	static_cast<float>(atof(chunks[1].c_str())),
									static_cast<float>(atof(chunks[2].c_str())),
									static_cast<float>(atof(chunks[3].c_str()))	);
			data.vertices.push_back(tempVertex);
		}
		else if (chunks[0] == "vt")
		{
			if (numChunks < 2)
			{
				ERROR_RECOVERABLE(Stringf("Not enough elements for texCoord, vt u [v w]: %s", line.c_str()));
				return false;
			}
			Vec2 tempUV;
			tempUV.x = static_cast<float>(atof(chunks[1].c_str()));
			if (numChunks > 2)  tempUV.y = static_cast<float>(atof(chunks[2].c_str()));
			data.texCoords.push_back(tempUV);
		}
		else if (chunks[0] == "vn")
		{
			if (numChunks < 4)
			{
				ERROR_RECOVERABLE(Stringf("Not enough elements for normal, vn x y z: %s", line.c_str()));
				return false;
			}
			Vec3 tempNormal = Vec3(	static_cast<float>(atof(chunks[1].c_str())),
									static_cast<float>(atof(chunks[2].c_str())),
									static_cast<float>(atof(chunks[3].c_str())));
			data.normals.push_back(tempNormal);
		}
		else if (chunks[0] == "f")
		{
			if (numChunks < 4)
			{
				ERROR_RECOVERABLE(Stringf("Not enough elements for face, f 1/2/3 3// 2/: %s", line.c_str()));
				return false;
			}

			std::vector<FaceElement> face;
			for (int i = 1; i < numChunks; ++i)
			{

				FaceElement faceElement;
				Strings faceElementStrings = SplitStringOnDelimiter(chunks[i], '/');
				int tempV = atoi(faceElementStrings[0].c_str());
				if (tempV == 0)
				{
					ERROR_RECOVERABLE(Stringf("Wrong face element, no vertex index: %s", line.c_str()));
					return false;
				}
				faceElement.v = tempV - 1;

				if (faceElementStrings.size() > 1)
				{
					int tempVt = atoi(faceElementStrings[1].c_str());
					faceElement.vt = tempVt - 1;
				}
				else
				{
					faceElement.vt = -1;
				}

				if (faceElementStrings.size() > 2)
				{
					int tempVn = atoi(faceElementStrings[2].c_str());
					faceElement.vn = tempVn - 1;
				}
				else
				{
					faceElement.vn = -1;
				}

				face.push_back(faceElement);
			}

			data.faces.push_back(face);
		}
	}


	// Building Triangles from read data
	// not check if you use a index out of range in face
	size_t numVerts = data.vertices.size();
	size_t numTexCoords = data.texCoords.size();
	size_t numNormals = data.normals.size();
	size_t numFaces = data.faces.size();

	for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex)
	{
		std::vector<FaceElement> const& faceElements = data.faces[faceIndex];

		for (int i = 0; i < (int)faceElements.size() - 2; ++i)
		{
			// Calculate for a triangle

			// Correct winding
			FaceElement faceTri0, faceTri1, faceTri2;
			if (isForwardCCW)
			{
				faceTri0 = faceElements[0];
				faceTri1 = faceElements[i + 1];
				faceTri2 = faceElements[i + 2];
			}
			else
			{
				faceTri0 = faceElements[0];
				faceTri1 = faceElements[i + 2];
				faceTri2 = faceElements[i + 1];
			}

			if (faceTri0.vt >= numTexCoords || faceTri1.vt >= numTexCoords || faceTri2.vt >= numTexCoords ||
				faceTri0.vn >= numNormals || faceTri1.vn >= numNormals || faceTri2.vn >= numNormals ||
				faceTri0.v >= numVerts || faceTri1.v >= numVerts || faceTri2.v >= numVerts)
			{
				ERROR_RECOVERABLE("Invalid Index for v, vt, vn");
				return false;
			}

			bool hasUVs = (faceTri0.vt >= 0) && (faceTri1.vt >= 0) && (faceTri2.vt >= 0);
			bool hasNormals = (faceTri0.vn >= 0) && (faceTri1.vn >= 0) && (faceTri2.vn >= 0);

			Vertex_PCUTBN tri0;
			Vertex_PCUTBN tri1;
			Vertex_PCUTBN tri2;

			tri0.m_position = data.vertices[faceTri0.v];
			tri1.m_position = data.vertices[faceTri1.v];
			tri2.m_position = data.vertices[faceTri2.v];

			if (!hasNormals && !hasUVs)
			{
				Vec3 faceNormal = CrossProduct3D(tri1.m_position - tri0.m_position, tri2.m_position - tri0.m_position).GetNormalized();

				Mat44 tbn = Mat44::MakeFromZ(faceNormal);
				Vec3 tangent = tbn.GetIBasis3D();
				Vec3 bitangent = tbn.GetJBasis3D();

				tri0.m_normal = faceNormal;
				tri1.m_normal = faceNormal;
				tri2.m_normal = faceNormal;

				tri0.m_tangent = tangent;
				tri1.m_tangent = tangent;
				tri2.m_tangent = tangent;

				tri0.m_bitangent = bitangent;
				tri1.m_bitangent = bitangent;
				tri2.m_bitangent = bitangent;

				out_verts.push_back(tri0);
				out_verts.push_back(tri1);
				out_verts.push_back(tri2);
				continue;
			}

			if (!hasUVs && hasNormals)
			{
				tri0.m_normal = data.normals[faceTri0.vn];
				tri1.m_normal = data.normals[faceTri1.vn];
				tri2.m_normal = data.normals[faceTri2.vn];

				Mat44 tbn0 = Mat44::MakeFromZ(tri0.m_normal);
				tri0.m_tangent = tbn0.GetIBasis3D();
				tri0.m_bitangent = tbn0.GetJBasis3D();

				Mat44 tbn1 = Mat44::MakeFromZ(tri1.m_normal);
				tri1.m_tangent = tbn1.GetIBasis3D();
				tri1.m_bitangent = tbn1.GetJBasis3D();

				Mat44 tbn2 = Mat44::MakeFromZ(tri2.m_normal);
				tri2.m_tangent = tbn2.GetIBasis3D();
				tri2.m_bitangent = tbn2.GetJBasis3D();

				out_verts.push_back(tri0);
				out_verts.push_back(tri1);
				out_verts.push_back(tri2);
				continue;
			}

			// Must has UVs
			tri0.m_uvTexCoords = data.texCoords[faceTri0.vt];
			tri1.m_uvTexCoords = data.texCoords[faceTri1.vt];
			tri2.m_uvTexCoords = data.texCoords[faceTri2.vt];

			Vec3 tangent;
			Vec3 bitangent;
			CalculateTangentBitangent(tangent, bitangent, tri0.m_position, tri1.m_position, tri2.m_position,
				tri0.m_uvTexCoords, tri1.m_uvTexCoords, tri2.m_uvTexCoords);

			tri0.m_tangent = tangent;
			tri1.m_tangent = tangent;
			tri2.m_tangent = tangent;

			tri0.m_bitangent = bitangent;
			tri1.m_bitangent = bitangent;
			tri2.m_bitangent = bitangent;

			if (hasNormals)
			{
				tri0.m_normal = data.normals[faceTri0.vn];
				tri1.m_normal = data.normals[faceTri1.vn];
				tri2.m_normal = data.normals[faceTri2.vn];
			}
			else
			{
				Vec3 faceNormal = CrossProduct3D(tangent, bitangent).GetNormalized();
				tri0.m_normal = faceNormal;
				tri1.m_normal = faceNormal;
				tri2.m_normal = faceNormal;
			}

			OrthonormalizeTB(tri0.m_tangent, tri0.m_bitangent, tri0.m_normal);
			OrthonormalizeTB(tri1.m_tangent, tri1.m_bitangent, tri1.m_normal);
			OrthonormalizeTB(tri2.m_tangent, tri2.m_bitangent, tri2.m_normal);

			out_verts.push_back(tri0);
			out_verts.push_back(tri1);
			out_verts.push_back(tri2);
		}
	}

	return true;
}
