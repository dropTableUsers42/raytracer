#ifndef _QTABLE_H
#define _QTABLE_H

#include <glad/glad.h>

#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/gtx/string_cast.hpp>

#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <random>

#include "BVH.hpp"

#define MAX_POINTS 100

#define MAX_THETA 10

#define MAX_PHI 10


/**
 * Stores all QTable relevant data
 */
class QTable
{
	std::stringbuf QTableBuffer, QVisitsBuffer; /**< Stringbuf to store initialized value of QTable and Visits array */


	/**
	 * Helper function to put any type of data into stringbuf
	 * @param buf Buffer to put data in
	 * @param var Data
	 * @return buf Buffer that data was put in
	 */
	template<class Type>
	std::stringbuf& put(std::stringbuf& buf, const Type& var);

	/**
	 * Helper function to extract any type of data into stringbuf
	 * @param buf Buffer to get data from
	 * @param[out] var Variable which will store the data after function call
	 * @return buf Buffer that data was extracted from
	 */
	template<class Type>
	std::stringbuf& get(std::stringbuf& buf, Type& var);
public:

	std::vector< std::vector<glm::vec3> > normal_per_voronoi; /** A 2D array storing the normal at the surface of each voronoi point, indexed by the objId and the index of the voronoi point on the unwrapped uv map of that object */

	std::vector< std::vector<glm::vec3> > voronoi_centres; /** A 2D array storing the position of each voronoi point, indexed by the objId and the index of the voronoi point on the unwrapped uv map of that object */
	
	QTable(int numObjs);

	/**
	 * Populates normals_per_voronoi and voronoi_centers
	 * @param triangles_per_obj Array of triangles for each object
	 */
	void makeVoronoi(std::vector< std::vector<Triangle> > triangles_per_obj);

	/**
	 * Initializes the QTable buffer (randomized), and the visits buffer (to zero)
	 */
	void fillQtableSsbo();

	/**
	 * Sends QTable and Visits data to GPU SSBOs
	 */
	void sendDataToGPU();

	GLuint QTableSsbo, QVisitsSsbo;
};

#endif