#include "BVH.hpp"

Triangle::Triangle()
{
	vertices.resize(3);
	normals.resize(3);
}

Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, int materialIndex) : materialIndex(materialIndex)
{
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
	normals.push_back(n1);
	normals.push_back(n2);
	normals.push_back(n3);
	uvs.push_back(uv1);
	uvs.push_back(uv2);
	uvs.push_back(uv3);
}

glm::vec3 Triangle::centroid()
{
	return (vertices[0] + vertices[1] + vertices[2]) * ONE_THIRD;
}

glm::vec3 Triangle::min()
{
	glm::vec3 minvec;
	minvec.x = std::min(std::min(vertices[0].x, vertices[1].x), vertices[2].x);
	minvec.y = std::min(std::min(vertices[0].y, vertices[1].y), vertices[2].y);
	minvec.z = std::min(std::min(vertices[0].z, vertices[1].z), vertices[2].z);
	return minvec;
}

glm::vec3 Triangle::max()
{
	glm::vec3 maxvec;
	maxvec.x = std::max(std::max(vertices[0].x, vertices[1].x), vertices[2].x);
	maxvec.y = std::max(std::max(vertices[0].y, vertices[1].y), vertices[2].y);
	maxvec.z = std::max(std::max(vertices[0].z, vertices[1].z), vertices[2].z);
	return maxvec;
}

void Triangle::calculateNormals()
{
	glm::vec3 side1 = vertices[1] - vertices[0];
	glm::vec3 side2 = vertices[2] - vertices[1];
	glm::vec3 normal = glm::normalize(glm::cross(side1, side2));
	normals[0] = normals[1] = normals[2] = normal;
}

BVH::BVH()
{
	parent = left = right = hitNext = missNext = NULL;
}

BVHContainer::BVHContainer(std::vector<Triangle> triangles, std::vector<Material*> materials, std::vector<Triangle> emitters) : triangles(triangles), materials(materials), emitters(emitters)
{
	root = buildBVH(this->triangles);
	buildNextLinks(root);
	allocateIndexes(root, indexes);
	bvhToSsbo();
	sendDataToGPU();
}

BVH* BVHContainer::buildBVH(std::vector<Triangle> &triangles) const
{
	BVH* node = new BVH;

	node->min = node->max = triangles[0].centroid();

	for(auto it = triangles.begin(); it != triangles.end(); it++)
	{
		(node->min).x = std::min((node->min).x, (it->min()).x);
		(node->min).y = std::min((node->min).y, (it->min()).y);
		(node->min).z = std::min((node->min).z, (it->min()).z);

		(node->max).x = std::max((node->max).x, (it->max()).x);
		(node->max).y = std::max((node->max).y, (it->max()).y);
		(node->max).z = std::max((node->max).z, (it->max()).z);
	}

	if(triangles.size() > MAX_TRIANGLES_IN_NODE)
	{
		glm::vec3 len = node->max - node->min;

		glm::vec3 centroid(0.0f);
		for(auto it = triangles.begin(); it != triangles.end(); it++)
		{
			centroid += it->centroid();
		}
		centroid /= triangles.size();

		std::vector<Triangle> left;
		std::vector<Triangle> right;

		if(len.x > len.y && len.x > len.z)
		{
			//xmax
			for(auto it = triangles.begin(); it != triangles.end(); it++)
			{
				if(it->centroid().x < centroid.x)
				{
					left.push_back(*it);
				}
				else
				{
					right.push_back(*it);
				}
			}
		}
		else if(len.y > len.z)
		{
			//ymax
			for(auto it = triangles.begin(); it != triangles.end(); it++)
			{
				if(it->centroid().y < centroid.y)
				{
					left.push_back(*it);
				}
				else
				{
					right.push_back(*it);
				}
			}
		}
		else
		{
			//zmax
			for(auto it = triangles.begin(); it != triangles.end(); it++)
			{
				if(it->centroid().z < centroid.z)
				{
					left.push_back(*it);
				}
				else
				{
					right.push_back(*it);
				}
			}
		}
		node->left = buildBVH(left);
		node->left->isLeft = true;
		node->left->parent = node;
		node->right = buildBVH(right);
		node->right->isLeft = false;
		node->right->parent = node;
	}
	else
	{
		node->triangles = triangles;
	}
	return node;
}

void BVHContainer::buildNextLinks(BVH *root)
{
	if(root->left != NULL)
	{
		buildNextLinks(root->left);
	}
	if(root->right != NULL)
	{
		buildNextLinks(root->right);
	}

	if(root->parent == NULL)
	{
		//root
		root->hitNext = root->left;
		root->missNext = NULL;
	}
	else if(root->isLeft && root->left == NULL)
	{
		//left leaf
		root->hitNext = root->missNext = root->parent->right;
	}
	else if(root->isLeft && root->left != NULL)
	{
		//left non-leaf
		root->hitNext = root->left;
		root->missNext = root->parent->right;
	}
	else if(!root->isLeft && root->left != NULL)
	{
		//right non-leaf
		root->hitNext = root->left;
		BVH *next = root->parent;
		while(!next->isLeft && next->parent != NULL)
		{
			next = next->parent;
		}
		if(next->parent != NULL)
		{
			root->missNext = next->parent->right;
		} else
		{
			root->missNext = NULL;
		}
		
	}
	else if(!root->isLeft && root->left == NULL)
	{
		BVH *next = root->parent;
		while(!next->isLeft && next->parent != NULL)
		{
			next = next->parent;
		}
		if(next->parent != NULL)
		{
			next = next->parent->right;
		}
		else
		{
			next = NULL;
		}
		root->missNext = root->hitNext = next;
		
	}
}

void BVHContainer::allocateIndexes(BVH *root, std::map<BVH*, int> &indexes)
{
	std::queue<BVH*> nodes;
	nodes.push(root);

	while(!nodes.empty())
	{
		BVH* node = nodes.front();
		nodes.pop();
		if(node == NULL)
			continue;
		
		int index = indexes.size();
		indexes.insert(std::pair<BVH*, int>(node, index));
		indexesVector.push_back(node);
		nodes.push(node->left);
		nodes.push(node->right);
	}
}

void BVHContainer::bvhToSsbo()
{
	int triangleIndex = 0;
	std::vector<GPUNode> gpuNodes;

	for(int i = 0; i < indexes.size(); i++)
	{
		BVH* node = indexesVector[i];
		GPUNode gnode;
		gnode.min[0] = node->min.x;
		gnode.min[1] = node->min.y;
		gnode.min[2] = node->min.z;
		gnode.max[0] = node->max.x;
		gnode.max[1] = node->max.y;
		gnode.max[2] = node->max.z;

		if(node->hitNext != NULL)
		{
			gnode.hitNext = indexes[node->hitNext];
		}
		else
		{
			gnode.hitNext = -1;
		}

		if(node->missNext != NULL)
		{
			gnode.missNext = indexes[node->missNext];
		}
		else
		{
			gnode.missNext = -1;
		}

		if(node->triangles.size() > 0)
		{
			gnode.firstTri = triangleIndex;
			gnode.numTris = node->triangles.size();
			triangleIndex += node->triangles.size();

			for(int i=0; i < node->triangles.size(); i++)
			{
				Triangle &t = node->triangles[i];

				put(put(put(put(trianglesBuffer, t.vertices[0].x), t.vertices[0].y), t.vertices[0].z), 1.0f);
				put(put(put(put(trianglesBuffer, t.vertices[1].x), t.vertices[1].y), t.vertices[1].z), 1.0f);
				put(put(put(put(trianglesBuffer, t.vertices[2].x), t.vertices[2].y), t.vertices[2].z), 1.0f);

				put(put(put(put(trianglesBuffer, t.normals[0].x), t.normals[0].y), t.normals[0].z), 0.0f);
				put(put(put(put(trianglesBuffer, t.normals[1].x), t.normals[1].y), t.normals[1].z), 0.0f);
				put(put(put(put(trianglesBuffer, t.normals[2].x), t.normals[2].y), t.normals[2].z), t.materialIndex);

				put(put(trianglesBuffer, t.uvs[0].x), t.uvs[0].y);
				put(put(trianglesBuffer, t.uvs[1].x), t.uvs[1].y);
				put(put(put(put(trianglesBuffer, t.uvs[2].x), t.uvs[2].y), 0.0f), 0.0f);
			}
		}
		else
		{
			gnode.firstTri = 0;
			gnode.numTris = 0;
		}
		
		gpuNodes.push_back(gnode);
		
	}

	for(int i = 0; i < gpuNodes.size(); i++)
	{
		put(put(nodesBuffer, gpuNodes[i].min), 1.0f);
		put(nodesBuffer, gpuNodes[i].max);
		put(nodesBuffer, gpuNodes[i].hitNext);
		put(nodesBuffer, gpuNodes[i].missNext);
		put(nodesBuffer, gpuNodes[i].firstTri);
		put(nodesBuffer, gpuNodes[i].numTris);
		put(nodesBuffer, 1.0f);
	}

	for(auto it = materials.begin(); it != materials.end(); it++)
	{
		put(put(put(put(mtlBuffer, ((*it)->Kd).x), ((*it)->Kd).y), ((*it)->Kd).z), (*it)->Ns);
		put(put(put(put(mtlBuffer, ((*it)->Ke).x), ((*it)->Ke).y), ((*it)->Ke).z), (*it)->emitter);
	}

	for(auto it = emitters.begin(); it != emitters.end(); it++)
	{
		Triangle t = *it;
		put(put(put(put(emittersBuffer, t.vertices[0].x), t.vertices[0].y), t.vertices[0].z), 1.0f);
		put(put(put(put(emittersBuffer, t.vertices[1].x), t.vertices[1].y), t.vertices[1].z), 1.0f);
		put(put(put(put(emittersBuffer, t.vertices[2].x), t.vertices[2].y), t.vertices[2].z), 1.0f);

		put(put(put(put(emittersBuffer, t.normals[0].x), t.normals[0].y), t.normals[0].z), 0.0f);
		put(put(put(put(emittersBuffer, t.normals[1].x), t.normals[1].y), t.normals[1].z), 0.0f);
		put(put(put(put(emittersBuffer, t.normals[2].x), t.normals[2].y), t.normals[2].z), t.materialIndex);

		put(put(emittersBuffer, t.uvs[0].x), t.uvs[0].y);
		put(put(emittersBuffer, t.uvs[1].x), t.uvs[1].y);
		put(put(put(put(emittersBuffer, t.uvs[2].x), t.uvs[2].y), 0.0f), 0.0f);
	}
}

template<class Type>
std::stringbuf& BVHContainer::put(std::stringbuf& buf, const Type& var)
{
	buf.sputn(reinterpret_cast<const char*>(&var), sizeof(var));
	return buf;
}

template<class Type>
std::stringbuf& BVHContainer::get(std::stringbuf& buf, Type& var)
{
	buf.sgetn(reinterpret_cast<char*>(&var), sizeof(var));
	return buf;
}

void BVHContainer::sendDataToGPU()
{
	glGenBuffers(1, &nodesSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, nodesSsbo);
	glBufferData(GL_ARRAY_BUFFER, nodesBuffer.str().length(), nodesBuffer.str().c_str(), GL_STATIC_DRAW);
	glGenBuffers(1, &trianglesSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, trianglesSsbo);
	glBufferData(GL_ARRAY_BUFFER, trianglesBuffer.str().length(), trianglesBuffer.str().c_str(), GL_STATIC_DRAW);
	glGenBuffers(1, &materialsSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, materialsSsbo);
	glBufferData(GL_ARRAY_BUFFER, mtlBuffer.str().length(), mtlBuffer.str().c_str(), GL_STATIC_DRAW);
	glGenBuffers(1, &emittersSsbo);
	glBindBuffer(GL_ARRAY_BUFFER, emittersSsbo);
	glBufferData(GL_ARRAY_BUFFER, emittersBuffer.str().length(), emittersBuffer.str().c_str(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}