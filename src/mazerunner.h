#ifndef MAZERUNNER_H_NORMALEXISTING
#define MAZERUNNER_H_NORMALEXISTING

#include <vector>
#include <cstdint>
#include <random>
#include <fstream>
#include <string>

#include "maze3d.h"
#include "hilbert.h"

#include <iostream>

/*

Utils to generate and step through mazes

*/

const float cardinals[4] = {0, M_PI * 0.5f, M_PI, M_PI * 1.5f};

// https://asgersvenning.com/maze-hilbert-lookahead/
Maze3d generateHilbertMaze(uint8_t order = 4, uint64_t seed = 0) {
	std::mt19937 prng(seed ? seed : rand());
	std::uniform_real_distribution<double> rand01(0, 1);
	if (order > 8) order = 8;
	int LEN = (1<<(order + 1)) - 1;
	int N = (1<<order);
	Maze3d maze(LEN, LEN);
	for (int i = 1; i < LEN; i += 2) {
		for (int j = 0; j < LEN; j++) maze.cells[i][j] = maze.cells[j][i] = 1;
	}

	Hilbert::hilbertMatrix hmat = Hilbert::hilbert(order);

	int dx[4] = {1, 0, -1, 0};
	int dy[4] = {0, 1, 0, -1};

	std::vector<int> dirs;
	for (int r = 0; r < N; r++) {
		for (int c = 0; c < N; c++) {
			dirs.clear();
			for (int d = 0; d < 4; d++) {
				
				int xp = r + dx[d];
				int yp = c + dy[d];
				if (xp < 0 || yp < 0 || xp >= N || yp >= N) continue;
				if (hmat[xp][yp] > hmat[r][c]) dirs.push_back(d);
			}
			if (dirs.size() <= 0) continue;
			int index = dirs.size() * rand01(prng);
			int dir = dirs[index];
			maze.cells[(r<<1) + dx[dir]][(c<<1) + dy[dir]] = 0;
		}
	}

	return maze;
}

namespace MazeAction {
	constexpr uint8_t STEP = 1;
	constexpr uint8_t RIGHT = 2;
	constexpr uint8_t BACK = 4;
	constexpr uint8_t LEFT = 8;
	constexpr uint8_t SPIN = 16;
};

// All maze action pickers will always change position, unless it can't, in which case SPIN is returned.
uint8_t rightHandRule(Maze3d& maze, int x, int y, int d) {
	const int dx[4] = {1, 0, -1, 0};
	const int dy[4] = {0, 1, 0, -1};
	int dr = (d + 3) & 3;
	int db = (d + 2) & 3;
	int dl = (d + 1) & 3;
	// if the right side is uncovered then we move forward
	if (!maze.get(x + dx[dr], y + dy[dr])) return MazeAction::RIGHT;
	// else try going straight
	if (!maze.get(x + dx[d], y + dy[d])) return MazeAction::STEP;
	// else try going left
	if (!maze.get(x + dx[dl], y + dy[dl])) return MazeAction::LEFT;
	// else try going back
	if (!maze.get(x + dx[db], y + dy[db])) return MazeAction::BACK;
	// spin idly
	return MazeAction::SPIN;
}

void renderMazeRunner(Maze3d& maze, uint8_t (*makeMove)(Maze3d&, int, int, int), int LEN = 128, int height = 64, int NMOVES = 1024, int FPM = 8, float FOV = SQUARE_FOV, int sx = 0, int sy = 0, std::string OUT_DIR = "video") {
	if (FPM < 0) FPM = 1;

	if (maze.get(sx, sy)) {
		for (int r = 0; r < maze.R; r++) {
			for (int c = 0; c < maze.C; c++) {
				if (!maze.get(r, c)) {
					sx = r;
					sy = c;
				}
			}
		}
	}
	const int dx[4] = {1, 0, -1, 0};
	const int dy[4] = {0, 1, 0, -1};
	int x = sx;
	int y = sy;
	int d = 1;

	float invFPM = 1.0f / FPM;
	float turnStep = invFPM * (M_PI * 0.5f);

	int framecount = 0;
	for (int mv = 0; mv < NMOVES; mv++) {
		float tx = x;
		float ty = y;
		float td = cardinals[d];
		uint8_t action = makeMove(maze, x, y, d);
		if (action & MazeAction::RIGHT) {
			int dp = (d + 3) & 3;

			for (int i = 0; i < FPM; i++) {
				std::string FILE_OUT = OUT_DIR + "/FRAME_" + std::to_string(framecount++);
				td -= turnStep;
				std::ofstream output(FILE_OUT);
				output << maze.renderBuffer(LEN, height, {tx + 0.5f, ty + 0.5f}, td, FOV);
				output.close();
			}
			d = dp;
		} if (action & MazeAction::LEFT) {
			int dp = (d + 1) & 3;

			for (int i = 0; i < FPM; i++) {
				std::string FILE_OUT = OUT_DIR + "/FRAME_" + std::to_string(framecount++);
				td += turnStep;
				std::ofstream output(FILE_OUT);
				output << maze.renderBuffer(LEN, height, {tx + 0.5f, ty + 0.5f}, td, FOV);
				output.close();
			}
			d = dp;
		} if (action & MazeAction::BACK) {
			int dp = (d + 1) & 3;

			for (int i = 0; i < FPM; i++) {
				std::string FILE_OUT = OUT_DIR + "/FRAME_" + std::to_string(framecount++);
				td += turnStep;
				std::ofstream output(FILE_OUT);
				output << maze.renderBuffer(LEN, height, {tx + 0.5f, ty + 0.5f}, td, FOV);
				output.close();
			}
			d = dp;
			if (x == sx && y == sy && d == 0) {
				std::cout << "LOOPED\n";
				break;
			}
			dp = (d + 1) & 3;

			for (int i = 0; i < FPM; i++) {
				std::string FILE_OUT = OUT_DIR + "/FRAME_" + std::to_string(framecount++);
				td += turnStep;
				std::ofstream output(FILE_OUT);
				output << maze.renderBuffer(LEN, height, {tx + 0.5f, ty + 0.5f}, td, FOV);
				output.close();
			}
			d = dp;
		}	
		if (x == sx && y == sy && d == 0) {
			std::cout << "LOOPED\n";
			break;
		}
		if (action && !(action & MazeAction::SPIN)) { // a non-spin action always moves forward after turning.
			int xp = x + dx[d];
			int yp = y + dy[d];

			for (int i = 0; i < FPM; i++) {
				std::string FILE_OUT = OUT_DIR + "/FRAME_" + std::to_string(framecount++);
				tx += dx[d] * invFPM;
				ty += dy[d] * invFPM;
				std::ofstream output(FILE_OUT);
				output << maze.renderBuffer(LEN, height, {tx + 0.5f, ty + 0.5f}, cardinals[d], FOV);
				output.close();
			}
			x = xp;
			y = yp;
		}
		if (mv % 16 == 0) std::cout << "MV " << (mv+1) << "/" << NMOVES << ":" << x << " " << y << " " << d << "\n";

		if (x == sx && y == sy && d == 0) {
			std::cout << "LOOPED\n";
			break;
		}
	}

	std::cout << "FRAMES: " << framecount << "\n";

	std::ofstream output(OUT_DIR + "/LEN");
	output << framecount;
	output.close();
}

#endif