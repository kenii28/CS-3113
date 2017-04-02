/*
Kenneth Huynh
CS 3113 Game Programming
Hw04
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
enum State { MENU, GAME_MODE, WIN, LOSEONE, LOSETWO };


SDL_Window* displayWindow;
ShaderProgram* program;
GLuint sprites2; 
GLuint textTexture; 

int state = 0;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float elapsed;



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

float lerp(float v0, float v1, float t) {
	return (float)(1.0 - t)*v0 + t*v1;
}

class Entity {
public:
	Entity() {}

	Entity(float x, float y, float width, float height, float speed_X, float speed_Y) {
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
		program->setModelMatrix(entityMatrix);
		sprite.Draw();
	}

	void Updated(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
			velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
			velocity.x += Acceleration.x * elapsed;
			velocity.y += Acceleration.y * elapsed;
			position.x += velocity.x * elapsed;
			dimension.l += velocity.x * elapsed;
			dimension.r += velocity.x * elapsed;
			position.y += velocity.y * elapsed;
			dimension.b += velocity.y * elapsed;
			dimension.t += velocity.y * elapsed;
		}
	}

	void UpdateX(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
			if (velocity.x < 5.0f && velocity.x > -5.0f) {
				velocity.x += Acceleration.x * elapsed;
			}
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
	EntityType Type;

	bool collidedT = false;
	bool collidedB = false;
	bool collidedL = false;
	bool collidedR = false;
};

SheetSprite platform;
SheetSprite snowman;
SheetSprite sun;
SheetSprite igloo;

Entity player;
Entity goal;
Entity ice;
Entity enemy1;
Entity enemy2;
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
	DrawText(program, "HURRY HOME SNOWMAN", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-5.0f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "ARROW KEYS TO MOVE SPACE TO START", 0.30f, 0.001f);
}

void RenderGame() {
	player.Render();
	goal.Render();
	enemy1.Render();
	enemy2.Render();
	for (size_t i = 0; i < platforms.size(); i++) {
		platforms[i].Render();
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0f);
	
	program->setViewMatrix(viewMatrix);
};

void RenderWin() {
	glClearColor(0.3f, 0.7f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity(); 
	modelMatrix.Translate(7.25f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU MADE IT HOME!", 0.5f, 0.0001f);
}

void RenderLose1() {
	glClearColor(0.3f, 0.7f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.0f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "OH NO YOU MELTED!", 0.5f, 0.0001f);
}

void RenderLose2() {
	glClearColor(0.3f, 0.7f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(2.25f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);

	DrawText(program, "OH NO YOU MELTED!", 0.5f, 0.0001f);
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
	case LOSEONE:
		RenderLose1();
		break;
	case LOSETWO:
		RenderLose2();
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
	
	if (player.collidedWith(goal)) {
		state = 2;
	}

	if (player.collidedWith(enemy1)) {
		state = 3;
	}
	
	if (player.collidedWith(enemy2)) {
		state = 4;
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
		break;
	case LOSEONE:
		break;
	case LOSETWO:
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

	sprites2 = LoadTexture("sprites2.png");
	textTexture = LoadTexture("font.png");

	platform = SheetSprite(sprites2, 0.0f / 512.0f, 289.0f / 512.0f, 240.0f / 512.0f, 53.0f / 512.0f, 0.5f);
	snowman = SheetSprite(sprites2, 122.0f / 512.0f, 344.0f / 512.0f, 105.0f / 512.0f, 129.0f / 512.0f, 0.7f);
	sun = SheetSprite(sprites2, 0.0f / 512.0f, 344.0f / 512.0f, 120.0f / 512.0f, 111.0f / 512.0f, 0.7f);
	igloo = SheetSprite(sprites2, 0.0f / 512.0f, 0.0f / 512.0f, 369.0f / 512.0f, 287.0f / 512.0f, 0.5f);
	

	player = Entity(-5.0f, 0.5f, 0.75f, 0.5f, 0.0f, 0.0f);
	player.sprite = snowman;
	player.Type = ENTITY_PLAYER;
	player.isStatic = false;
	player.Acceleration.y = -5.0f;

	goal = Entity(11.5f, 0.5f, 0.4f, 0.8f, 0.0f, 0.0f);
	goal.sprite = igloo;
	goal.Type = ENTITY_GOAL;

	enemy1 = Entity(2.0f, 0.5f, 0.4f, 0.8f, 0.0f, 0.0f);
	enemy1.sprite = sun;
	enemy1.Type = ENTITY_ENEMY;

	enemy2 = Entity(6.0f, 0.5f, 0.4f, 0.8f, 0.0f, 0.0f);
	enemy2.sprite = sun;
	enemy2.Type = ENTITY_ENEMY;
	

	for (size_t i = 0; i < 33; i++) {
		ice = Entity(-5.33f + (i)* 0.47f, 0.0f, 0.5f, 0.5f, 0.8f, 0.08f);
		ice.sprite = platform;
		platforms.push_back(ice);
	}
	
	float lastFrameTicks = 0.0f;
	bool done = false;

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
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					state = 1;
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
		/*SPEED HACKS
		else if (keys[SDL_SCANCODE_RIGHT]) {
			player.velocity.x = 11.5;
		}*/
		else if (keys[SDL_SCANCODE_UP]) {
			if (player.collidedB == true) {
				player.velocity.y = 3.5f;
			}
		}

		glClearColor(0.3f, 0.7f, 1.0f, 0.0f);
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
		Update(fixedElapsed);
		Render();
	}
	SDL_Quit();
	return 0;
}