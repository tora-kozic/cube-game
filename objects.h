
#ifndef OBJECTS_H
#define OBJECTS_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int grid_width = 32;	// y
int grid_height = 32;	// z
int grid_depth = 16;	// x
float grid_scale = 0.3;

struct CollisionData;

struct Model; // contains model data
struct ObjData; // contains data for each dynamic rendered object
struct ColData; // contains collision data for each object

struct BlockData {
	int blocks[16][32][32] = {};

	BlockData() {}

	int getXCollision(glm::vec3 p, glm::vec3 i) { // next player position, corresponding indices
		if (!CheckIndexValid(i)) {
			return 0;
		}
		if (blocks[(int)i.x][(int)i.y][(int)i.z] == 0 && blocks[(int)i.x + 1][(int)i.y][(int)i.z] == 0) { // check if index of next position is open
			
			return 0;
		}
		int x_midpoint;
		float x_boundary;
		x_midpoint = grid_depth / 2 - 1;
		x_boundary = grid_scale * (i.x - x_midpoint);
		if (p.x < x_boundary + .05) return -1;
		return 0;
	}

	int getYCollision(float vely, glm::vec3 curr_p, glm::vec3 curr_i, glm::vec3 p, glm::vec3 i) { // current player position, next player position, corresponding indices
		int next_y = i.y;
		if (i.y == curr_i.y) {
			if (vely < 0) next_y = curr_i.y - 1;
			else if (vely > 0) next_y = curr_i.y + 1;
		}

		if (i.y < 0 || i.y > grid_width - 1) return 0;  // if outside range

		float y_center, y_midpoint;
		if (blocks[(int)i.x+1][next_y][(int)i.z] != 0) {
				y_midpoint = grid_width / 2;
				y_center = grid_scale * (next_y - y_midpoint);
				if (abs(p.y - y_center) < .15) {
					return -1;
				}
		}
		return 0;
	}

	int getZCollision(float velz, glm::vec3 curr_p, glm::vec3 curr_i, glm::vec3 p, glm::vec3 i) { // current player position, next player position, corresponding indices
		int next_z = i.z;
		if (i.z == curr_i.y) {
			if (velz < 0) next_z = curr_i.z - 1;
			else if (velz > 0) next_z = curr_i.z + 1;
		}

		if (i.z < 0 || i.z > grid_height - 1) return 0;  // if outside range

		float z_center, z_midpoint;

		if (blocks[(int)i.x + 1][(int)i.y][next_z] != 0) {
			z_midpoint = grid_height / 2;
			z_center = grid_scale * (i.z - z_midpoint);
			if (abs(p.z - z_center) < .15) {
				return -1;
			}
		}
		return 0;
	}

	static bool CheckIndexValid(glm::vec3 i) {
		if (i.x < 0 || i.x > grid_depth) return false;
		if (i.y < 0 || i.y > grid_width) return false;
		if (i.z < 0 || i.z > grid_height) return false;
		return true;
	}
};

struct Model {
	int startIndex;
	int numVerts;
	float* verts;

	Model() {}

	Model(int s, int n, float* v) : startIndex(s), numVerts(n), verts(v) {}
};

struct ObjData {
	float objx, objy, objz;
	float colR, colG, colB;
	float velx, vely, velz;
	float angleSpeed, linSpeed;
	float playerRadius, pickupRadius;
	bool init = false;
	bool equipped = false;
	bool used = false;

	ObjData() {
		objx = 0;
		objy = 0;
		objz = 0;
		colR = 1;
		colG = 1;
		colB = 1;
		velx = 0;
		vely = 0;
		velz = 0;
		angleSpeed = 0;
		linSpeed = 0;
		playerRadius = 0;
		pickupRadius = 0;
	}

	ObjData(float x, float y, float z, float r, float g, float b, float a_speed, float l_speed, float p_radius, float pick_radius) {
		objx = x;
		objy = y;
		objz = z;
		colR = r;
		colG = g;
		colB = b;
		velx = 0;
		vely = 0;
		velz = 0;
		angleSpeed = a_speed;
		linSpeed = l_speed;
		playerRadius = p_radius;
		pickupRadius = pick_radius;
	}
};

#endif // OBJECTS_H