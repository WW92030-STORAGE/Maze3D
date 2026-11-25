#ifndef MAZE3D_H_NORMALEXISTING
#define MAZE3D_H_NORMALEXISTING

#include <vector>
#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>
#include <random>
#include <array>

#include <iostream>


// your code here

const float SQUARE_FOV = 2 * atan(0.5);

constexpr bool isFloatZero(float f) {
	constexpr float E = 1e-5;
	return -E < f && f < E;
}

constexpr bool isZeroOne(float f) {
	return isFloatZero(f) || isFloatZero(f - 1);
}

inline float normalizeAngle(float f) {
	constexpr float TAU = 2.0f * M_PI;
	float g = fmod(f, TAU);
	if (g < -M_PI) g += TAU;
	if (g > M_PI) g -= TAU;
	return g;
}

// UTILS

std::string u8tohex(uint8_t x) {
	std::string res = "xx";
	uint8_t unit = x & 15;
	res[1] = (unit >= 10) ? ('A' + (unit - 10)) : ('0' + unit);
	x>>=4;
	res[0] = (x >= 10) ? ('A' + (x - 10)) : ('0' + x);
	return res;
}

uint32_t rgb32(uint8_t r, uint8_t g, uint8_t b) {
	return 255 | ((uint32_t)(r)<<24) | ((uint32_t)(g)<<16) | ((uint32_t)(b)<<8);
}

std::array<uint8_t, 4> decodeRGBA(uint32_t x) {
	return {(uint8_t)(x>>24), (uint8_t)((x>>16)&255), (uint8_t)((x>>8)&255), (uint8_t)(x&255)};
}

uint32_t rgba(std::array<uint8_t, 4> x) {
	return (uint32_t)(x[3]) | ((uint32_t)(x[0])<<24) | ((uint32_t)(x[1])<<16) | ((uint32_t)(x[2])<<8);
}

uint32_t blend(uint32_t bg, uint32_t ontop) {
	std::array<uint8_t, 4> bgRGBA = decodeRGBA(bg);
	std::array<uint8_t, 4> ontopRGBA = decodeRGBA(ontop);
	float p = ontopRGBA[3] / 255.0f;
	float q = 1.0f - p;
	for (int i = 0; i < 4; i++) bgRGBA[i] = q * bgRGBA[i] + p * ontopRGBA[i];
	return rgba(bgRGBA);
}

std::string rgba32tohex(uint32_t x) {
	std::string res = "xxxxxxxx";
	for (int i = 7; i >= 0; i--) {
		uint32_t unit = x & 15;
		res[i] = (unit >= 10) ? ('A' + (unit - 10)) : ('0' + unit);
		x>>=4;
	}
	return res;
}

uint32_t scaleColor(uint32_t color, float f) {
	if (f < 0.0f) f = 0.0f;
	if (f > 1.0f) f = 1.0f;
	uint32_t r = (color>>24)&255;
	uint32_t g = (color>>16)&255;
	uint32_t b = (color>>8)&255;
	uint32_t a = color & 255;
	return ((uint32_t(r * f))<<24) | ((uint32_t(g * f))<<16) | ((uint32_t(b * f))<<8) | a;
}

struct Maze3dVec2 {
	float x = 0;
	float y = 0;

	std::string to_string() {
		return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
	}

	Maze3dVec2 operator+(const Maze3dVec2& o) {
		return {x + o.x, y + o.y};
	}
	Maze3dVec2 operator-(const Maze3dVec2& o) {
		return {x - o.x, y - o.y};
	}
	Maze3dVec2 operator*(float f) {
		return {x * f, y * f};
	}

	float normsquared() {
		return x * x + y * y;
	}
	float length() {
		return sqrtf(normsquared());
	}
	float angle() {
		return normalizeAngle(atan2(y, x));
	}
	float dot(Maze3dVec2 o) {
		return x * o.x + y * o.y;
	}
	Maze3dVec2 normalized() {
		float len = length();
		if (isFloatZero(len)) return {x, y};
		float inv = 1.0f / len;
		return {x * inv, y * inv};
	}
};

struct Maze3dTexture {
	std::vector<std::vector<uint32_t>> tex;
	uint32_t color = 0xFFFFFFFF;
	bool imageTexture = false;

	Maze3dTexture(uint32_t col = 0xFFFFFFFF) { color = col; }
	Maze3dTexture(std::vector<std::vector<uint32_t>> xx) { tex = xx; imageTexture = true; }
	
	uint32_t get(Maze3dVec2 uv) {
		if (!imageTexture) return color;
		uv.x -= floorf(uv.x);
		uv.y -= floorf(uv.y);
		int x = tex.size() * uv.x;
		int y = tex[0].size() * uv.y;
		return tex[x][y];
	}
};

struct Maze3dColumn {
	float height = 1.0f;
	uint8_t id = 0;
	float texU = 0;
	float depth = 0;
	bool isVertical = false;
	std::string disp() {
		return "[r=" + std::to_string(height) + ", id=" + std::to_string(id) + ", u=" + std::to_string(texU) + "d=" + std::to_string(depth) + "v=" + std::to_string(isVertical) + "]";
	}
	std::string to_string() { return "Maze3dColumn" + disp(); }
};

// Takes an input and a unit vector, find out how much is needed to move to hit the next horizontal or vertical grid line
float raymarch(Maze3dVec2 v, Maze3dVec2 u = {1.0f, 0.0f}) {
	float remx = v.x - floorf(v.x);
	float remy = v.y - floorf(v.y);
	if (u.x > 0) remx = 1.0f - remx;
	else if (isFloatZero(remx)) remx = 1.0f;
	if (u.y > 0) remy = 1.0f - remy;
	else if (isFloatZero(remy)) remy = 1.0f;

	float relx = 1.0f;
	float rely = 1.0f;
	if (!isFloatZero(u.x)) relx = remx / fabs(u.x);
	if (!isFloatZero(u.y)) rely = remy / fabs(u.y);
	return std::min(relx, rely);
}

// Billboarding system

struct Maze3dCharacter {
	uint8_t texId = 0; 
	float width = 0.5f;
	float height = 0.5f;
	Maze3dVec2 pos;
	float verticalPosition = 0.0f;
};

/*

The maze is positioned on a RxC grid, spanning the interval [0, R] x [0, C] in the x and y directions, respectively.
Viewing the maze from above, in the right hand rule, is facing the negative Z direction, and you are located in positive Z.
The view angle is measured counterclockwise from +x.

*/

struct Maze3d {
	float NEAR_PLANE = 0.01;
	uint32_t FLOOR = 0x808080FF;
	uint32_t CEIL = 0xC0C0C0FF;
	uint32_t BORDER = 0x404040FF; // The border has texture id -1 or 255.
	bool useBorder = true;
	float verticalShade = 0.8;

	uint16_t R;
	uint16_t C;
	float renderDistance;
	int maxIterations;
	std::vector<std::vector<uint8_t>> cells;
	std::vector<Maze3dTexture> textures;
	
	std::vector<Maze3dCharacter> characters;

	Maze3d(int r = 16, int c = 16) {
		R = r;
		C = c;
		cells = std::vector<std::vector<uint8_t>>(R, std::vector<uint8_t>(C, 0));
		renderDistance = R + C;
		maxIterations = R + C;

		// The texture of cell with id [id] is given by textures[id - 1]. The id 0 is air.
		textures.push_back(Maze3dTexture(0xFFFFFFFF));
		textures.push_back(Maze3dTexture(0xFF0000FF));
		textures.push_back(Maze3dTexture(0xFFFF00FF));
		textures.push_back(Maze3dTexture(0x00FF00FF));
		textures.push_back(Maze3dTexture(0x00FFFFFF));
		textures.push_back(Maze3dTexture(0x0000FFFF));
		textures.push_back(Maze3dTexture(0xFF00FFFF));
	}

	uint8_t get(int r, int c) { 
		if (r < 0 || c < 0 || r >= R || c >= C) return -1;
		return cells[r][c];
	}
	uint8_t get(Maze3dVec2 v) {
		return get(floorf(v.x), floorf(v.y));
	}

	uint32_t getTexel(uint8_t id, float u, float v) {
		if (id == (uint8_t)(-1) && useBorder) return BORDER;
		if (id <= 0) return 0;
		if (id > textures.size()) return 0;
		return textures[id - 1].get({u, 1.0f - v});
	}

	void setAllWalls(uint8_t x) {
		for (int r = 0; r < R; r++) {
			for (int c = 0; c < C; c++) {
				if (cells[r][c]) cells[r][c] = x;
			}
		}
	}

	void setRandomWalls(uint8_t x, uint64_t seed = 0) {
		std::mt19937 prng(seed ? seed : rand());
		std::uniform_int_distribution<uint8_t> dis(0, x - 1);
		for (int r = 0; r < R; r++) {
			for (int c = 0; c < C; c++) {
				if (cells[r][c]) cells[r][c] = 1 + dis(prng);
			}
		}
	}

	Maze3dColumn castRay(Maze3dVec2 v = {0.5f, 0.5f}, float theta = M_PI * 0.25f, float centerTheta = M_PI * 0.25) {
		float dtheta = fabs(theta - centerTheta);
		if (dtheta >= M_PI * 0.5f) return Maze3dColumn{0, 0};

		Maze3dVec2 dir = {cosf(theta), sinf(theta)};
		Maze3dVec2 epsilon = dir * 0.0001;
		float totalDistance = 0;

		for (int iter = 0; iter < maxIterations; iter++) {
			if (totalDistance > renderDistance) break;
			Maze3dVec2 huss = v + epsilon;
			if (uint8_t color = get(huss)) {
				float height = 0.5f;
				float orthoDist = totalDistance * cosf(dtheta);
				if (!isFloatZero(orthoDist)) height = 0.5f / orthoDist;

				float computedU = 0;
				float xfrac = v.x - floorf(v.x);
				float yfrac = v.y - floorf(v.y);
				bool isVertical = false;
				if (isZeroOne(xfrac)) {
					computedU = (dir.x > 0) ? (1.0f - yfrac) : yfrac;
					isVertical = true;
				}
				else if (isZeroOne(yfrac)) computedU = (dir.y > 0) ? xfrac : (1.0 - xfrac);
				return Maze3dColumn{height, color, computedU, orthoDist, isVertical};
			}
			float dist = raymarch(v, dir);
			totalDistance += dist;
			v = v + dir * dist;
		}
		return Maze3dColumn{0, 0};
	}

	std::vector<Maze3dColumn> generateColumns(Maze3dVec2 v = {0.5f, 0.5f}, int len = 64, float viewAngle = M_PI * 0.25f, float FOV = SQUARE_FOV) {
		viewAngle = fmod(viewAngle, M_PI * 2.0f);
		if (len <= 0) len = 1;
		std::vector<Maze3dColumn> res(len);


		float dtheta = FOV / len;
		float theta = viewAngle - (len - 1) * 0.5 * dtheta;
		for (int i = len - 1; i; i--) {
			res[i] = castRay(v, theta, viewAngle);
			theta += dtheta;
		}
		return res;
	}

	/*
	
	A note on the vertical segments:
	There are N horizontal bars, thus each bar spans [FOV / N] angle, and the angular distance between bars is FOV / N.
	There are (height) vertical bars. Each bar needs to span FOV / N angle.
	But the way things are now, the stored height of each column is 1.0 / (orthoDist).

	For a 1mx1m square 1 meter away from the player and located at the center of the view, the width is marked as W*N bars (W being the width it occupies in the square view). 
	The height should also be W of the screen. But the rendered height is 1.

	So, we scale the heights by a vertical scaling factor W.
	What is W? W = 2 * atan(2) / FOV.
		
	*/

	// characters begin on the floor -- that means 0.5/(orthodist) below the horizon line (height>>1). 
	void drawCharacters(std::vector<std::vector<uint32_t>>& buffer, std::vector<Maze3dColumn>& columns, Maze3dVec2 v, float viewAngle, float FOV) {
		// std::cout << "FRAME: " << v.to_string() << " " << viewAngle << " " << FOV << "\n";
		viewAngle = normalizeAngle(viewAngle);
		if (FOV <= 0) return;
		float invFOV = 1.0f / FOV;
		int N = buffer.size();
		int height = buffer[0].size();
		float VERTICAL_SCALING_FACTOR = SQUARE_FOV / FOV;
		std::vector<std::pair<float, int>> indices;
		for (int si = 0; si < characters.size(); si++) {
			Maze3dCharacter spr = characters[si];
			Maze3dVec2 disp = spr.pos - v;
			float dtheta = normalizeAngle(disp.angle() - viewAngle);
			float cosine = cosf(dtheta);
			if (cosine <= 0) continue;
			float dist = disp.length();
			float orthoDist = dist * cosine;
			// std::cout << si << " rawtheta=" << disp.angle() - viewAngle << " -- dtheta=" << dtheta << " d=" << dist << " cos=" << cosine << " ortho=" << orthoDist << "\n";
			if (orthoDist <= NEAR_PLANE) continue;
			indices.push_back({-orthoDist, si});
		}
		std::sort(indices.begin(), indices.end());
		for (int xi = 0; xi < indices.size(); xi++) {
			int si = indices[xi].second;
			Maze3dCharacter spr = characters[si];
			Maze3dVec2 disp = spr.pos - v;
			float dtheta = normalizeAngle(disp.angle() - viewAngle);
			/*
			float cosine = cosf(dtheta);
			if (cosine < 0) continue;
			float dist = disp.length();
			float orthoDist = dist * cosine;
			*/
			float orthoDist = -indices[xi].first;

			float invOD = 1.0f / orthoDist;

			float floorDiameter = VERTICAL_SCALING_FACTOR * invOD;
			float floorRadius = floorDiameter * 0.5f;
			int trueFloor = (height>>1) - floorRadius * N;
			int trueCeil = (height>>1) + floorRadius * N;

			// 1 is the width of the square view.
			float relHeight = spr.height * floorDiameter;
			float baseYOffset = (floorRadius - spr.verticalPosition * relHeight) * N;
			float relWidth = spr.width * invOD;
			float relX = 0.5f - dtheta * invFOV;
			int baseX = (relX - relWidth * 0.5f) * N;
			// std::cout << xi << " " << si << " = dtheta=" << dtheta << "rx=" << relX << "w=" << relWidth << "h=" << relHeight << "bx" << baseX << "od=" << orthoDist << "\n";
			if (baseX + relX * N <= -N || baseX >= N) continue;
			float invWN = 1.0f / (relWidth * N);
			float invWH = 1.0f / (relHeight * N);
			for (int r = std::max(0, -baseX - 1); r < relWidth * N; r++) {
				int x = r + baseX;
				if (x < 0) continue;
				if (x >= N) break;
				if (orthoDist >= columns[x].depth) continue;
				for (int c = std::max(0, int(baseYOffset) - (height>>1) - 1); c < relHeight * N; c++) {
					int y = (height>>1) - baseYOffset + c;
					if (y < trueFloor || y < 0) continue;
					if (y >= trueCeil || y >= height) break;
					
					uint32_t bg = buffer[x][y];
					uint32_t ontop = getTexel(spr.texId, r * invWN, c * invWH);
					buffer[x][y] = blend(bg, ontop);
				}
			}
		}
	}

	std::vector<std::vector<uint32_t>> buffer(std::vector<Maze3dColumn>& columns, int height = 0, float FOV = SQUARE_FOV) {
		int N = columns.size();
		if (height <= 0) height = N;
		std::vector<std::vector<uint32_t>> val(N, std::vector<uint32_t>(height, 0));

		float VERTICAL_SCALING_FACTOR = SQUARE_FOV / FOV;

		for (int r = 0; r < N; r++) {
			for (int c = 0; c < height; c++) {
				val[r][c] = (c < (height>>1)) ? FLOOR : CEIL;
			}
			if (isFloatZero(columns[r].height)) continue;
			float trueHeight = columns[r].height * (N<<1) * VERTICAL_SCALING_FACTOR;
			int dispHeight = trueHeight;
			int radius = dispHeight>>1;
			uint8_t id = columns[r].id;
			float invH = 1.0f / dispHeight;
			float u = columns[r].texU;
			for (int c = 0; c < dispHeight; c++) {
				int y = (height>>1) - radius + c;
				if (y >= 0 && y < height) {
					val[r][y] = getTexel(id, u, invH * (c + 0.5f));
					if (columns[r].isVertical) val[r][y] = scaleColor(val[r][y], verticalShade);
				}
			}
		}

		return val;
	}

	// image width, image height, player position, player direction, player FOV
	std::string renderBuffer(int N = 64, int height = 0, Maze3dVec2 v = {0.5f, 0.5f}, float viewAngle = M_PI * 0.25f, float FOV = SQUARE_FOV) {
		viewAngle = fmod(viewAngle, M_PI * 2.0f);
		if (height <= 0) height = N;
		std::vector<Maze3dColumn> columns = generateColumns(v, N, viewAngle, FOV);
		std::vector<std::vector<uint32_t>> buf = buffer(columns, height, FOV);

		// Render sprites
		drawCharacters(buf, columns, v, viewAngle, FOV);

		std::string res = "[" + std::to_string(N) + ", " + std::to_string(height) + "]";
		for (int c = height - 1; c >= 0; c--) {
			res.push_back('\n');
			for (int r = 0; r < N; r++) res += std::to_string(buf[r][c]) + ",";
		}
		return res;
	}

	std::string to_string() {
		std::string res = "Maze3d[" + std::to_string(R) + ", " + std::to_string(C) + "]";
		for (int c = C - 1; c >= 0; c--) {
			res.push_back('\n');
			for (int r = 0; r < R; r++) res += u8tohex(cells[r][c]) + " ";
		}
		return res;
	}
	std::string monochrome() {
		std::string res = "Maze3d[" + std::to_string(R) + ", " + std::to_string(C) + "]";
		for (int c = C - 1; c >= 0; c--) {
			res.push_back('\n');
			for (int r = 0; r < R; r++) res.push_back(cells[r][c] ? '#' : '.');
		}
		return res;
	}
};

// Misc utils

std::string disp(std::vector<Maze3dColumn>& mc) {
	std::string res = "[";
	for (int i = 0; i < mc.size(); i++) res += mc[i].disp();
	return res + "]";
}

#endif
