/*
Kenneth Huynh
CS 3113 Game Programming
Final Project
Platformer

*/

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <iostream>
#include <math.h> 
#include <SDL_mixer.h>


#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_BLOCK, ENTITY_GOAL };
enum State { MENU, GAME_MODE, WIN, BTWN12, LEVELTWO, BTWN23, LEVELTHREE, GAMEOVER };


SDL_Window* displayWindow;
ShaderProgram* program;
GLuint knight; 
GLuint block;
GLuint textTexture; 
GLuint stuff;

int state = 0;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float elapsed;
const int runAnimation[] = { 1, 2, 3, 4, 5};
const int numFrames = 5;
float animationElapsed = 0.0f;
float framesPerSecond = 6.0f;
int currentIndex = 0;
bool presslv2 = false;
bool presslv3 = false;



class Vector3 {
public:
	Vector3() {}
	Vector3(float a, float b, float c) : x(a), y(b), z(c) {}
	float x;
	float y;
	float z;
};

class Vector4 {
public:
	Vector4() {}
	Vector4(float a, float b, float c, float d) : t(a), b(b), l(c), r(d) {}
	float t;
	float b;
	float l;
	float r;
};

class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(GLuint textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};
	void Draw() {
		glBindTexture(GL_TEXTURE_2D, textureID);
		float texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, -0.5f * size
		};
		glUseProgram(program->programID);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	};
	float size;
	GLuint textureID;
	float u;
	float v;
	float width;
	float height;
};
void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0f / (float)spriteCountX;
	float spriteHeight = 1.0f / (float)spriteCountY;
	GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth,
		v + spriteHeight };
	float vertices[] = {
		-0.25f, -0.25f,
		0.25f, 0.25f,
		-0.25f, 0.25f,
		0.25f, 0.25f,
		-0.25f, -0.25f,
		0.25f, -0.25f };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, knight);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//Clean
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

float lerp(float v0, float v1, float t) {
	return (float)(1.0 - t)*v0 + t*v1;
}

class Entity {
public:
	Entity() {}

	Entity(float x, float y, float width, float height, float speed_X, float speed_Y, float scale) {
		position.x = x;
		position.y = y;
		dimension.t = y + height / 2;
		dimension.b = y - height / 2;
		dimension.l = x - width / 2;
		dimension.r = x + width / 2;
		velocity.x = speed_X;
		velocity.y = speed_Y;
		Acceleration.x = 0.0f;
		Acceleration.y = 0.0f;
		friction.x = 0.1f;
		friction.y = 0.0f;
		size.x = width;
		size.y = height;
		scaling = scale;
	}

	bool collidedWith(const Entity &object) {
		if (!(dimension.b > object.dimension.t ||
			dimension.t < object.dimension.b ||
			dimension.r < object.dimension.l ||
			dimension.l > object.dimension.r)) {
			return true;
		}
		return false;
	}

	void Render() {
		entityMatrix.identity();
		entityMatrix.Translate(position.x, position.y, 0);
		if (isStatic && (Type == ENTITY_BLOCK)) {
			entityMatrix.Scale(10.0f * scaling, 0.75f * scaling, scaling);
		}
		if (isStatic && (Type == ENTITY_GOAL)) {
			entityMatrix.Scale(scaling, scaling, scaling);
		}
		else if (!isStatic && (Type == ENTITY_ENEMY)) {
			entityMatrix.Scale(scaling, scaling, scaling);
		}
		else {
			entityMatrix.Scale(scaling, scaling, scaling);
		}
		program->setModelMatrix(entityMatrix);
		if (Type == ENTITY_PLAYER){
			DrawSpriteSheetSprite(program, runAnimation[currentIndex], 7, 4);
		}
		else{
			sprite.Draw();
		}
		
		
	}

	void UpdateX(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
			velocity.x += Acceleration.x * elapsed;
			position.x += velocity.x * elapsed;
			dimension.l += velocity.x * elapsed;
			dimension.r += velocity.x * elapsed;
		}
	}
	void UpdateY(float elapsed) {
		if (!isStatic) {
			velocity.y += Acceleration.y * elapsed;
			position.y += velocity.y * elapsed;
			dimension.t += velocity.y * elapsed;
			dimension.b += velocity.y * elapsed;
		}
	}

	SheetSprite sprite;

	Vector3 position;
	Vector3 size;
	Vector4 dimension;
	Vector3 velocity;
	Vector3 Acceleration;
	Vector3 friction;
	Matrix entityMatrix;

	bool isStatic = true;
	float scaling;
	EntityType Type;

	bool collidedT = false;
	bool collidedB = false;
	bool collidedL = false;
	bool collidedR = false;
};

SheetSprite knightsprite;
SheetSprite groundsprite;
SheetSprite castlesprite;
SheetSprite spikesprite;
SheetSprite batsprite;

Entity player;
Entity ground;
Entity castle;

Entity goal;
Entity spike;
Entity bat;
vector<Entity> platforms;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}

void DrawText(ShaderProgram *program, string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, textTexture);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void RenderMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-3.5f, 1.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "RUN HOME KNIGHT", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-5.0f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "Help the knight get home!", 0.30f, 0.001f);


	modelMatrix.identity();
	modelMatrix.Translate(-5.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "ARROW KEYS TO MOVE SPACE TO START", 0.30f, 0.001f);
}

void RenderGame() {
	ground = Entity(-3.0f, -2.0f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	ground.sprite = groundsprite;
	ground.Type = ENTITY_BLOCK;
	platforms.push_back(ground);
	//}

	//for (size_t i = 0; i < 1; i++) {
	ground = Entity(-3.0f, -2.0f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	ground.sprite = groundsprite;
	ground.Type = ENTITY_BLOCK;
	platforms.push_back(ground);

	castle = Entity(0.0f, -1.1f, 0.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	castle.sprite = castlesprite;
	castle.Type = ENTITY_GOAL;
	player.Render();
	castle.Render();
	for (size_t i = 0; i < platforms.size(); i++) {
		platforms[i].Render();
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0f);
	
	program->setViewMatrix(viewMatrix);
};

void RenderGame2() {
	platforms[0] = Entity(-3.0f, -2.0f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	platforms[0].sprite = groundsprite;
	platforms[0].Type = ENTITY_BLOCK;

	platforms[1] = Entity(4.00f, -2.0f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	platforms[1].sprite = groundsprite;
	platforms[1].Type = ENTITY_BLOCK;

	castle = Entity(9.0f, -1.1f, 0.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	castle.sprite = castlesprite;
	castle.Type = ENTITY_GOAL;

	player.Render();
	castle.Render();
	
	for (size_t i = 0; i < platforms.size(); i++) {
		platforms[i].Render();
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0f);

	program->setViewMatrix(viewMatrix);
}

void RenderGame3() {
	platforms[0] = Entity(-3.0f, -2.0f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	platforms[0].sprite = groundsprite;
	platforms[0].Type = ENTITY_BLOCK;

	platforms[1] = Entity(3.66f, -1.5f, 5.5f, 0.5f, 0.8f, 0.08f, 1.0f);
	platforms[1].sprite = groundsprite;
	platforms[1].Type = ENTITY_BLOCK;

	castle = Entity(6.0f, -0.5f, 0.7f, 0.5f, 0.8f, 0.08f, 1.0f);
	castle.sprite = castlesprite;
	castle.Type = ENTITY_GOAL;


	player.Render();
	castle.Render();
	bat.Render();

	for (size_t i = 0; i < platforms.size(); i++) {
		platforms[i].Render();
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0f);

	program->setViewMatrix(viewMatrix);
}


void Render12() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	
	DrawText(program, "YOU BEAT LEVEL 1", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "PRESS SPACE FOR", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -3.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "LEVEL 2", 0.5f, 0.0001f);
	
	viewMatrix.identity();
	viewMatrix.Translate(-2.0f, 1.0f, 0.0f);

	program->setViewMatrix(viewMatrix);
}

void Render23() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "YOU BEAT LEVEL 2", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "PRESS SPACE FOR", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -3.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "LEVEL 3", 0.5f, 0.0001f);

	viewMatrix.identity();
	viewMatrix.Translate(-2.0f, 1.0f, 0.0f);

	program->setViewMatrix(viewMatrix);
}

void RenderWin() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "YOU BEAT THE GAME", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "  WOOOOOOOOOOOOO", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -3.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "     YOU WIN", 0.5f, 0.0001f);

	viewMatrix.identity();
	viewMatrix.Translate(-2.0f, 1.0f, 0.0f);

	program->setViewMatrix(viewMatrix);
}

void RenderGameOver() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "OH NO YOU DIED", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "PLEASE PRESS ESC", 0.5f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, -3.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "AND START OVER", 0.5f, 0.0001f);

	viewMatrix.identity();
	viewMatrix.Translate(-2.0f, 1.0f, 0.0f);

	program->setViewMatrix(viewMatrix);
}
void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case MENU:
		RenderMenu();
		break;
	case GAME_MODE:
		RenderGame();
		break;
	case WIN:
		RenderWin();
		break;
	case BTWN12:
		Render12();
		break;
	case LEVELTWO:
		RenderGame2();
		break;
	case BTWN23:
		Render23();
		break;
	case LEVELTHREE:
		RenderGame3();
		break;
	case GAMEOVER:
		RenderGameOver();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}


void UpdateGame(float elapsed) {
	float penetration;

	player.collidedT = false;
	player.collidedB = false;
	player.collidedL = false;
	player.collidedR = false;

	player.UpdateY(elapsed);
	for (size_t i = 0; i < platforms.size(); i++) {
		if (player.collidedWith(platforms[i])) {
			float y_distance = fabs(player.position.y - platforms[i].position.y);
			penetration = fabs(y_distance - player.size.y / 2.0f - platforms[i].size.y / 2.0f) + 0.00001f;
			if (player.position.y > platforms[i].position.y) {
				player.position.y += penetration;
				player.dimension.b += penetration;
				player.dimension.t += penetration;
				player.collidedB = true;
				player.velocity.y = 0.0f;
			}
			else {
				player.position.y -= penetration;
				player.dimension.b -= penetration;
				player.dimension.t -= penetration;
				player.collidedT = true;
				player.velocity.y = 0;
			}
			player.velocity.y = 0.0f;
		}
	}

	player.UpdateX(elapsed);
	for (size_t i = 0; i < platforms.size(); i++) {
		if (player.collidedWith(platforms[i])) {
			float x_distance = fabs(player.position.x - platforms[i].position.x);
			penetration = fabs(x_distance - player.size.x / 2.0f - platforms[i].size.x / 2.0f) + 0.00001f;
			if (player.position.x > platforms[i].position.x) {
				player.position.x += penetration;
				player.dimension.l += penetration;
				player.dimension.r += penetration;
				player.collidedL = true;
				player.velocity.x = 0;
				player.Acceleration.x = 0.0f;
			}
			else {
				player.position.x -= penetration;
				player.dimension.l -= penetration;
				player.dimension.r -= penetration;
				player.collidedR = true;
				player.velocity.x = 0;
				player.Acceleration.x = 0.0f;
			}
			player.velocity.x = 0.0f;
			player.Acceleration.x = 0.0f;
		}
	}

	if (state == LEVELTHREE) {
		bat.UpdateY(elapsed);
		for (size_t i = 0; i < platforms.size(); i++) {
			if (bat.collidedWith(platforms[i])) {
				float y_distance = fabs(bat.position.y - platforms[i].position.y);
				penetration = fabs(y_distance - bat.size.y / 2.0f - platforms[i].size.y / 2.0f) + 0.00001f;
				if (bat.position.y > platforms[i].position.y) {
					bat.position.y += penetration;
					bat.dimension.b += penetration;
					bat.dimension.t += penetration;
					bat.collidedB = true;
					bat.velocity.y = 0.0f;
				}
				else {
					bat.position.y -= penetration;
					bat.dimension.b -= penetration;
					bat.dimension.t -= penetration;
					bat.collidedT = true;
					bat.velocity.y = 0;
				}
				bat.velocity.y = 0.0f;
			}
		}

		bat.UpdateX(elapsed);
		for (size_t i = 0; i < platforms.size(); i++) {
			if (bat.collidedWith(platforms[i])) {
				float x_distance = fabs(bat.position.x - platforms[i].position.x);
				penetration = fabs(x_distance - bat.size.x / 2.0f - platforms[i].size.x / 2.0f) + 0.00001f;
				if (bat.position.x > platforms[i].position.x) {
					bat.position.x += penetration;
					bat.dimension.l += penetration;
					bat.dimension.r += penetration;
					bat.collidedL = true;
					bat.velocity.x = 0;
					bat.Acceleration.x = 0.0f;
				}
				else {
					bat.position.x -= penetration;
					bat.dimension.l -= penetration;
					bat.dimension.r -= penetration;
					bat.collidedR = true;
					bat.velocity.x = 0;
					bat.Acceleration.x = 0.0f;
				}
				bat.velocity.x = 0.0f;
				bat.Acceleration.x = 0.0f;
			}
		}
	}
	
	if (state == 1 && player.collidedWith(castle)) {
		state = BTWN12;
	}
	if (state == BTWN12 && presslv2) {
		state == LEVELTWO;
	}
	if (state == LEVELTWO && player.collidedWith(castle)) {
		state = BTWN23;
	}
	if (state == BTWN23 && presslv3) {
		state == LEVELTHREE;
	}
	if (state == LEVELTHREE && player.collidedWith(castle)) {
		state = WIN;
	}
	if (state == LEVELTHREE && player.collidedWith(bat)) {
		state = GAMEOVER;
	}
	if (player.position.y < -10.0f) {
		state == GAMEOVER;
	}
	if (bat.position.x < 3.6f) {
		bat.Acceleration.x = 0.3f;
	}
	if (bat.position.x > 4.5f) {
		bat.Acceleration.x = -0.3f;
	}
}


void Update(float elapsed) {
	switch (state)
	{
	case MENU:
		break;
	case GAME_MODE:
		UpdateGame(elapsed);
		break;
	case WIN:
		RenderWin();
		break;
	case BTWN12:
		Render12();
		break;
	case LEVELTWO:
		UpdateGame(elapsed);
		break;
	case BTWN23:
		Render23();
		break;
	case LEVELTHREE:
		UpdateGame(elapsed);
		break;
	case GAMEOVER:
		RenderGameOver();
		break;
	}

}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	SDL_Event event;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, 1280, 720);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	
	projectionMatrix.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	knight = LoadTexture("knight.png");
	block = LoadTexture("sprites3.png");
	stuff = LoadTexture("sprites4.png");
	textTexture = LoadTexture("font.png");

	knightsprite = SheetSprite(knight, 0.0f / 600.0f, 289.0f / 636.0f, 80.0f / 600.0f, 150.0f / 636.0f, 0.5f);
	groundsprite = SheetSprite(block, 0.0f, 0.0f, 1024.0f / 2048.0f, 204.0f / 256.0f, 1.0f);
	castlesprite = SheetSprite(stuff, 0.0f, 0.0f, 356.0f / 512.0f, 366.0f / 512.0f, 1.0f);
	spikesprite = SheetSprite(stuff, 246.0f / 512.0f, 368.0f / 512.0f, 98.0f / 512.0f, 135.0f / 512.0f, 1.0f);
	batsprite = SheetSprite(stuff, 0.0f / 512.0f, 368.0f / 512.0f, 244.0f / 512.0f, 128.0f / 512.0f, 0.75f);

	

	player = Entity(-4.5f, 0.5f, 0.75f, 0.5f, 0.0f, 0.0f, 2.0f);
	player.sprite = knightsprite;
	player.Type = ENTITY_PLAYER;
	player.isStatic = false;
	player.Acceleration.y = -5.0f;
	
	float lastFrameTicks = 0.0f;
	bool done = false;

	int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	
	Mix_Chunk *playerjump;
	playerjump = Mix_LoadWAV("playerjump.wav");
	Mix_Music *music;
	music = Mix_LoadMUS("music.mp3");
	

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					player.Acceleration.x = 0.0f;
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					done = true;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (state == 0) {
						state = 1;
						Mix_PlayMusic(music, -1);
					}
					if (state == BTWN12) {
						state = LEVELTWO;
						presslv2 = true;
						player = Entity(-4.5f, 0.5f, 0.75f, 0.5f, 0.0f, 0.0f, 2.0f);
						player.sprite = knightsprite;
						player.Type = ENTITY_PLAYER;
						player.isStatic = false;
						player.Acceleration.y = -5.0f;
					}
					if (state == BTWN23) {
						state = LEVELTHREE;
						presslv2 = true;
						player = Entity(-4.5f, 0.5f, 0.75f, 0.5f, 0.0f, 0.0f, 2.0f);
						player.sprite = knightsprite;
						player.Type = ENTITY_PLAYER;
						player.isStatic = false;
						player.Acceleration.y = -5.0f;

						bat = Entity(4.0f, -1.0f, 0.15f, 0.15f, 0.1f, 0.08f, 0.5f);
						bat.sprite = spikesprite;
						spike.Type = ENTITY_ENEMY;
						bat.isStatic = false;
						bat.Acceleration.y = -5.0f;
						bat.Acceleration.x = -0.3f;
					}
				}
			}
		}

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_LEFT]) {
			player.Acceleration.x = -2.5f;
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			player.Acceleration.x = 2.5;
		}
		else if (keys[SDL_SCANCODE_UP]) {
			if (player.collidedB == true) {
				Mix_PlayChannel(-1, playerjump, 0);
				player.velocity.y = 2.5f;
			}
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}

		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			Update(FIXED_TIMESTEP);
		}
		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			currentIndex++;
			animationElapsed = 0.0;
			if (currentIndex > numFrames - 1) {
				currentIndex = 0;
			}
		}

		Update(fixedElapsed);
		Render();
	}
	
	Mix_FreeChunk(playerjump);
	Mix_FreeMusic(music);

	SDL_Quit();
	return 0;
}