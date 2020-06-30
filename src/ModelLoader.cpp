#include "ModelLoader.hpp"

ModelLoader::ModelLoader(const char* path)
{
	loadObj(path);
}

void ModelLoader::loadObj(const char* path)
{
	std::ifstream file(path);
	std::string line;

	std::string pathroot(path);
	std::reverse(pathroot.begin(), pathroot.end());
	pathroot = pathroot.substr(pathroot.find("/") + 1);
	std::reverse(pathroot.begin(), pathroot.end());

	if(!file)
	{
		std::cerr << "ERROR::FILE::COULD NOT OPEN FILE FOR MODEL LOADING" << std::endl;
		return;
	}

	std::string curMtl("");
	int curObj = -1;

	while(!getline(file, line).eof())
	{
		if(line.substr(0, line.find(" ")) == "o")
		{
			curObj++;
			triangles_per_obj.resize(triangles_per_obj.size() + 1);
		}
		if(line.substr(0, line.find(" ")) == "v")
		{
			glm::vec3 vertex;
			sscanf(line.substr(line.find(" ")).c_str(), "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		if(line.substr(0, line.find(" ")) == "vt")
		{
			glm::vec2 uv;
			sscanf(line.substr(line.find(" ")).c_str(), "%f %f", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		if(line.substr(0, line.find(" ")) == "vn")
		{
			glm::vec3 normal;
			sscanf(line.substr(line.find(" ")).c_str(), "%f %f %f", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		if(line.substr(0, line.find(" ")) == "f")
		{
			std::string vertex1, vertex2, vertex3;
			unsigned long long indexes[9];
			std::string tmpstr(line.substr(line.find(" ")));
			for(int i=0; i < 9; i++)
			{
				std::string indexStr;
				if((i+1)%3 == 0)
				{
					indexStr = tmpstr.substr(0, tmpstr.find(" "));
					tmpstr = tmpstr.substr(tmpstr.find(" ") + 1);
				}
				else
				{
					indexStr = tmpstr.substr(0, tmpstr.find("/"));
					tmpstr = tmpstr.substr(tmpstr.find("/") + 1);
				}
				if(indexStr != "")
					indexes[i] = std::stoi(indexStr.c_str());
				else
					indexes[i] = 0;
				
			}
			vertexIndices.push_back(indexes[0]);
			vertexIndices.push_back(indexes[3]);
			vertexIndices.push_back(indexes[6]);
			uvIndices.push_back(indexes[1]);
			uvIndices.push_back(indexes[4]);
			uvIndices.push_back(indexes[7]);
			normalIndices.push_back(indexes[2]);
			normalIndices.push_back(indexes[5]);
			normalIndices.push_back(indexes[8]);
			if(curMtl != "")
			{
				material_per_vertex.push_back(materialIndexes[curMtl]);
			}
			else
			{
				material_per_vertex.push_back(-1);
			}
			objID_per_vertex.push_back(curObj);
			
		}
		if(line.substr(0, line.find(" ")) == "mtllib")
		{
			std::string mtlpath(line.substr(line.find(" ") + 1));
			mtlpath = pathroot + "/" + mtlpath;
			loadMat(mtlpath.c_str());
		}
		if(line.substr(0, line.find(" ")) == "usemtl")
		{
			curMtl = line.substr(line.find(" ") + 1);
		}
	}
	for(int i=0; i<vertexIndices.size(); i++)
	{
		bool v=false,n=false;
		if(vertexIndices[i] > 0)
		{
			glm::vec3 vertex = temp_vertices[vertexIndices[i]-1];
			out_vertices.push_back(vertex);
		}
		if(uvIndices[i] > 0)
		{
			glm::vec2 uv = temp_uvs[uvIndices[i]-1];
			out_uvs.push_back(uv);
		}
		if(normalIndices[i] > 0)
		{
			glm::vec3 normal = temp_normals[normalIndices[i]-1];
			out_normals.push_back(normal);
		}
		if((i+1)%3 == 0)
		{
			Triangle t(out_vertices[i-2], out_vertices[i-1], out_vertices[i], out_normals[i-2], out_normals[i-1], out_normals[i], out_uvs[i-2], out_uvs[i-1], out_uvs[i], material_per_vertex[i/3], objID_per_vertex[i/3]);
			triangles.push_back(t);
			if(materialsVector[material_per_vertex[i/3]]->emitter)
			{
				emitters.push_back(t);
			}
			triangles_per_obj[objID_per_vertex[i/3]].push_back(t);
		}
	}
	numObjs = curObj + 1;
}

void ModelLoader::loadMat(const char *path)
{

	std::ifstream file(path);
	std::string line;

	if(!file)
	{
		std::cerr << "ERROR::FILE::COULD NOT OPEN FILE FOR MODEL LOADING" << std::endl;
		return;
	}

	std::string curMtl;

	while(!getline(file, line).eof())
	{
		if(line.substr(0, line.find(" ")) == "newmtl")
		{
			curMtl = line.substr(line.find(" ") + 1);
			Material * mtl = new Material();
			materials.insert(std::pair<std::string, Material*>(curMtl, mtl));
			materialsVector.push_back(mtl);
			materialIndexes.insert(std::pair<std::string, int>(curMtl, materialsVector.size()-1));
		}
		if(line.substr(0, line.find(" ")) == "Kd")
		{
			line = line.substr(line.find(" ") + 1);
			glm::vec3 Kd;
			for(int i=0; i < 3; i++)
			{
				std::string f_str = line.substr(0, line.find(" "));
				float f = std::stof(f_str.c_str());
				Kd[i] = f;
				line = line.substr(line.find(" ") + 1);
			}
			materials[curMtl]->Kd = Kd;
		}
		if(line.substr(0, line.find(" ")) == "Ns")
		{
			std::string Ns_str = line.substr(line.find(" ") + 1);
			float f = std::stof(Ns_str.c_str());
			materials[curMtl]->Ns = f;
		}
		if(line.substr(0, line.find(" ")) == "Ke")
		{
			line = line.substr(line.find(" ") + 1);
			glm::vec3 Ke;
			for(int i=0; i < 3; i++)
			{
				std::string f_str = line.substr(0, line.find(" "));
				float f = std::stof(f_str.c_str());
				Ke[i] = f;
				line = line.substr(line.find(" ") + 1);
			}
			materials[curMtl]->Ke = Ke;
			if(Ke.x > 0.0 || Ke.y > 0.0 || Ke.z > 0.0)
				materials[curMtl]->emitter = true;
		}
	}
}