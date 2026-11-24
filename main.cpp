#include "src/maze3d.h"
#include "src/mazerunner.h"

#include <bits/stdc++.h>
using namespace std;



#include "texture_texture.h"

void testThings() {
	Maze3dVec2 v{0.2, 0.4};
	Maze3dVec2 dir{1, -1.1};
	dir = dir.normalized();
	cout << dir.to_string() << endl;
	float rm = raymarch(v, dir);
	cout << rm << endl;
	cout << (v + dir * rm).to_string() << endl;
}

void simplerender() {
	Maze3d maze(16, 16);
	maze.textures[1] = Maze3dTexture(std::vector<std::vector<uint32_t>>({{0xFFFFFFFF, 0xFF}, {0xFF, 0xFFFFFFFF}}));
	maze.textures[1] = Maze3dTexture(Maze3dTex::protos);
	maze.cells[10][10] = 2;
	maze.cells[10][12] = 2;
	maze.cells[12][10] = 2;
	maze.cells[10][8] = 2;

	cout << maze.renderBuffer(1024, 1024, {8.5f, 8.5f}) << endl;
}

void runMaze() {
	srand(0);
	Maze3d maze = generateHilbertMaze(3);
	maze.textures.push_back(Maze3dTexture(Maze3dTex::protos));
	maze.setRandomWalls(8);
	// maze.textures[0] = Maze3dTexture(Maze3dTex::animationsingleframe);
	// maze.textures[1] = Maze3dTexture(Maze3dTex::protos);
	cout << maze.monochrome() << endl;
	
	// cout << maze.renderBuffer(1024, 512, {16.5f, 16.5f}, M_PI / 2) << endl;

	renderMazeRunner(maze, rightHandRule, 128, 64);
}

void billboardtest() {
	Maze3d maze(16, 16);
	maze.textures[0] = Maze3dTexture(rgb32(255, 255, 255));
	maze.textures[1] = Maze3dTexture(std::vector<std::vector<uint32_t>>({{rgb32(160, 128, 128), rgb32(128, 160, 160)}, {rgb32(128, 160, 160), rgb32(160, 128, 128)}}));
	maze.textures[2] = Maze3dTexture(Maze3dTex::protogen);
	maze.cells[10][10] = 1;
	maze.cells[10][12] = 2;
	maze.cells[12][10] = 2;

	maze.characters.push_back(Maze3dCharacter{3, 0.6, 1, {9.5, 9.7}});
	// maze.characters.push_back(Maze3dCharacter{3, 0.6, 1, {11.3, 10}});

	cout << maze.renderBuffer(1024, 1024*3/4, {8.5f, 8.5f}) << endl;
}

int main() {
	billboardtest();
	// runMaze();
	return 0;
}
