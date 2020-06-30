#include "QTable.hpp"

float radicalInverse(unsigned int bits)
{
	bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

glm::vec2 hammersley(unsigned int i, unsigned int N = MAX_POINTS)
{
	return glm::vec2(float(i)/float(N), radicalInverse(i));
}

unsigned int nearest(glm::vec2 uv, unsigned int N = MAX_POINTS)
{
	float nearest = glm::length(hammersley(0, N) - uv);
	unsigned int nearestIdx = 0;
	for(unsigned int i = 1; i < N; i++)
	{
		float len = glm::length(hammersley(i, N) - uv);
		if(len < nearest)
		{
			nearestIdx = i;
			nearest = len;
		}
	}
	return nearestIdx;
}

QTable::QTable(int numObjs)
{
	srand(time(NULL));
	normal_per_voronoi.resize(numObjs);
	voronoi_centres.resize(numObjs);
}

template<class Type>
std::stringbuf& QTable::put(std::stringbuf& buf, const Type& var)
{
	buf.sputn(reinterpret_cast<const char*>(&var), sizeof(var));
	return buf;
}

template<class Type>
std::stringbuf& QTable::get(std::stringbuf& buf, Type& var)
{
	buf.sgetn(reinterpret_cast<char*>(&var), sizeof(var));
	return buf;
}

void QTable::makeVoronoi(std::vector< std::vector<Triangle> > triangles_per_obj)
{
	for(int i = 0; i < MAX_POINTS; i++)
	{
		glm::vec2 hamm = hammersley(i, MAX_POINTS);
		for(int j = 0; j < triangles_per_obj.size(); j++)
		{
			bool found = false;
			for(auto it = triangles_per_obj[j].begin(); it != triangles_per_obj[j].end(); it++)
			{
				glm::vec3 e1 = glm::vec3(it->uvs[1] - it->uvs[0], 0.0);
				glm::vec3 e2 = glm::vec3(it->uvs[2] - it->uvs[0], 0.0);
				glm::vec3 n = glm::cross(e1, e2);
				glm::vec3 edge1 = glm::vec3(it->uvs[1] - it->uvs[0], 0.0);
				glm::vec3 p1 = glm::vec3(hamm - it->uvs[0], 0.0);
				glm::vec3 edge2 = glm::vec3(it->uvs[2] - it->uvs[1], 0.0);
				glm::vec3 p2 = glm::vec3(hamm - it->uvs[1], 0.0);
				glm::vec3 edge3 = glm::vec3(it->uvs[0] - it->uvs[2], 0.0);
				glm::vec3 p3 = glm::vec3(hamm - it->uvs[2], 0.0);

				glm::vec3 c1 = cross(edge1, p1);
				glm::vec3 c2 = cross(edge2, p2);
				glm::vec3 c3 = cross(edge3, p3);

				if(glm::dot(c1, c2) > 0 && glm::dot(c2, c3) > 0 && glm::dot(c3, c1) > 0)
				{
					normal_per_voronoi[j].push_back(glm::normalize(glm::cross(it->vertices[1] - it->vertices[0], it->vertices[2] - it->vertices[0])));
					
					float u = glm::dot(n, c2) / glm::dot(n, n);
					float v = glm::dot(n, c3) / glm::dot(n, n);

					glm::vec3 pos = (1-u-v) * it->vertices[0] + u * it->vertices[1] + v * it->vertices[2];

					voronoi_centres[j].push_back(pos);

					found = true;

					break;
				}

			}
			if(!found)
			{
				normal_per_voronoi[j].push_back(glm::vec3(0.0, 0.0, 0.0));
				voronoi_centres[j].push_back(glm::vec3(0.0, 0.0, 0.0));
			}
		}
	}

	fillQtableSsbo();
	sendDataToGPU();

}

void QTable::fillQtableSsbo()
{
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<> dist(0,1);
	for(int i = 0; i < voronoi_centres.size(); i++)
	{
		for(int j = 0; j < voronoi_centres[i].size(); j++)
		{
			for(int k = 0; k < MAX_THETA; k++)
			{
				for(int l = 0; l < MAX_PHI; l++)
				{
					float f = dist(e2);
					put(QTableBuffer, f);
				}
			}
		}
	}

	for(int i = 0; i < voronoi_centres.size(); i++)
	{
		for(int j = 0; j < voronoi_centres[i].size(); j++)
		{
			put(QVisitsBuffer, (int)0);
		}
	}
}

void QTable::sendDataToGPU()
{
	glGenBuffers(1, &QTableSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, QTableSsbo);
	glBufferData(GL_ARRAY_BUFFER, QTableBuffer.str().length(), QTableBuffer.str().c_str(), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &QVisitsSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, QVisitsSsbo);
	glBufferData(GL_ARRAY_BUFFER, QVisitsBuffer.str().length(), QVisitsBuffer.str().c_str(), GL_STATIC_DRAW);
}