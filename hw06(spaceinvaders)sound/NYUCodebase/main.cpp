/*
Kenneth Huynh
CS 3113 Game Programming
Hw06
Space Invaders WITH SOUNDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD

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
#include <SDL_mixer.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

SDL_Window* displayWindow;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
ShaderProgram* program;
GLuint textTexture;
GLuint sprites;

enum GameState { MENU, GAME };
float elapsed;
bool moveL = false;
bool moveR = false;
bool shoot = false;
float lastFrameTicks = 0.0f;
float lastaShot = 0.0f;
float lastpShot = 0.0f;
int state = 0;
bool playing = true;



GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
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
class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(GLuint textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};
	void Draw() {
		glBindTexture(GL_TEXTURE_2D, sprites);
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

class Entity{
public:
	float pos[2];
	float tblr[4];
	float direction[2];
	float u;
	float v;
	float width;
	float height;
	float size = 1.0f;
	Matrix mat;
	SheetSprite sprite;

	Entity() {}

	Entity(float x, float y, float width, float height, float dirX, float dirY) {
		pos[0] = x;
		pos[1] = y;
		u = u;
		v = v;
		width = width;
		height = height;
		direction[0] = dirX;
		direction[1] = dirY;

		tblr[0] = y + 0.05f * size;
		tblr[1] = y - 0.05f * size;
		tblr[2] = x - 0.05f * size;
		tblr[3] = x + 0.05f * size;
		mat.identity();
		mat.Translate(x, y, 0);
	}

	void draw() {
		mat.identity();
		mat.Translate(pos[0], pos[1], 0);
		program->setModelMatrix(mat);
		sprite.Draw();
	}
};

SheetSprite dragon;
SheetSprite baddragon;
SheetSprite fire;
SheetSprite efire;

void RenderMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-2.5f, 1.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "DRAGON INVADERS", 0.40f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.7f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "PRESS SPACE TO START", 0.40f, 0.0001f);
}

Entity player;
Entity enemy;
Entity bullet;
vector<Entity> aliens;
vector<Entity> pLasers;
vector<Entity> aLasers;

void RenderGame() {
	player.draw();
	for (size_t i = 0; i < aliens.size(); i++) {
		aliens[i].draw();
	}
	for (size_t i = 0; i < pLasers.size(); i++) {
		pLasers[i].draw();
	}
	for (size_t i = 0; i < aLasers.size(); i++) {
		aLasers[i].draw();
	}
}

void UpdateMainMenu() {}
void updateGame(float elapsed) {
	if (moveL) {
		player.pos[0] -= player.direction[0] * elapsed;
		player.tblr[2] -= player.direction[0] * elapsed;
		player.tblr[3] -= player.direction[0] * elapsed;
	}
	else if (moveR) {
		player.pos[0] += player.direction[0] * elapsed;
		player.tblr[2] += player.direction[0] * elapsed;
		player.tblr[3] += player.direction[0] * elapsed;
	}
	if (shoot) {
		if (lastpShot > 0.5f){
			lastpShot = 0.0f;
			bullet = Entity(player.pos[0], player.pos[1], 360.0f / 1024.0f, 387.0f / 1024.0f, 0, 2.0f);
			bullet.sprite = fire;
			pLasers.push_back(bullet);
		}
	}
	for (size_t i = 0; i < aliens.size(); i++) {	
		aliens[i].pos[1] -= aliens[i].direction[1] * elapsed;
		aliens[i].tblr[0] -= aliens[i].direction[1] * elapsed;
		aliens[i].tblr[1] -= aliens[i].direction[1] * elapsed;

		aliens[i].pos[0] -= aliens[i].direction[0] * elapsed;
		aliens[i].tblr[2] -= aliens[i].direction[0] * elapsed;
		aliens[i].tblr[3] -= aliens[i].direction[0] * elapsed;

		if (aliens[i].tblr[2] < -5.0f || aliens[i].tblr[3] > 5.0f) {
			for (size_t i = 0; i < aliens.size(); i++) {
				aliens[i].direction[0] = -aliens[i].direction[0];
			}
		}
		if (aliens[i].tblr[0] > player.tblr[1] && aliens[i].tblr[1] < player.tblr[0] &&
			aliens[i].tblr[2] < player.tblr[3] && aliens[i].tblr[3] > player.tblr[2]) {
			playing = false;
		}

	}

	vector<int> deletepLaser;
	for (size_t i = 0; i < pLasers.size(); i++) {
		pLasers[i].pos[1] += pLasers[i].direction[1] * elapsed;
		pLasers[i].tblr[0] += pLasers[i].direction[1] * elapsed;
		pLasers[i].tblr[1] += pLasers[i].direction[1] * elapsed;
		/*if (pLasers[i].pos[1] > 2.5f) {
			deletepLaser.push_back(i);
		}*/

		for (size_t j = 0; j < aliens.size(); j++) {
			if (aliens[j].tblr[0] > pLasers[i].tblr[1] &&
				aliens[j].tblr[1] < pLasers[i].tblr[0] &&
				aliens[j].tblr[2] < pLasers[i].tblr[3] &&
				aliens[j].tblr[3] > pLasers[i].tblr[2]) {
				aliens.erase(aliens.begin() + j);
				deletepLaser.push_back(i);
				for (int i = 0; i < deletepLaser.size(); i++) {
					pLasers.erase(pLasers.begin() + deletepLaser[i] - i);
				}
			}
		}
	}

	// piercing bullets
	/*for (int i = 0; i < deletepLaser.size(); i++) { 
		pLasers.erase(pLasers.begin() + deletepLaser[i] - i);
	}*/

	if (lastaShot > 0.5f) {
		lastaShot = 0.0f;
		int randomAlien = rand() % aliens.size();
		bullet = Entity(aliens[randomAlien].pos[0], aliens[randomAlien].pos[1], 258.0f / 1024.0f, 237.0f / 1024.0f, 0, -2.0f);
		bullet.sprite = efire;
		aLasers.push_back(bullet);
	}

	for (size_t i = 0; i < aLasers.size(); i++) {
		aLasers[i].pos[1] += aLasers[i].direction[1] * elapsed;
		aLasers[i].tblr[0] += aLasers[i].direction[1] * elapsed;
		aLasers[i].tblr[1] += aLasers[i].direction[1] * elapsed;
		if (aLasers[i].tblr[0] > player.tblr[1] &&
			aLasers[i].tblr[1] < player.tblr[0] &&
			aLasers[i].tblr[2] < player.tblr[3] &&
			aLasers[i].tblr[3] > player.tblr[2]) {
			playing = false;
		}
		if (aLasers[i].pos[1] < -3.0f) {
			aLasers.erase(aLasers.begin() + i);
		}
	}

	if (aliens.size() == 0) {
		playing = false;
	}
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case MENU:
		RenderMenu();
		break;
	case GAME:
		RenderGame();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}
void Update(float elapsed) {
	switch (state) {
	case MENU:
		UpdateMainMenu();
		break;
	case GAME:
		updateGame(elapsed);
		break;
	}
}
//void ProcessInput() {
//	switch (state) {
//	case STATE_MAIN_MENU:
//		ProcessMainMenuInput();
//		break;
//	case STATE_GAME_LEVEL:
//		ProcessGameLevelInput();
//		break;
//	}
//}




int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1080, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	bool done = false;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, 1080, 720);

	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sprites = LoadTexture(RESOURCE_FOLDER"sprites.png");
	textTexture = LoadTexture(RESOURCE_FOLDER"text.png");

	dragon = SheetSprite(sprites, 0.0f / 1024.0f, 0.0f / 1024.0f, 498.0f / 1024.0f, 314.0f / 1024.0f, 0.7f);
	baddragon = SheetSprite(sprites, 0.0f / 1024.0f, 316.0f / 1024.0f, 475.0f / 1024.0f, 242.0f / 1024.0f, 0.7f);
	fire = SheetSprite(sprites, 0.0f / 1024.0f, 560.0f / 1024.0f, 360.0f / 1024.0f, 387.0f / 1024.0f, 0.5f);
	efire = SheetSprite(sprites, 362.0f / 1024.0f, 560.0f / 1024.0f, 258.0f / 1024.0f, 237.0f / 1024.0f, 0.5f);
	
	player = Entity(0.0f, -2.5f, 498.0f / 1024.0f, 314.0f / 1024.0f, 2.0f, 0.0f);
	player.sprite = dragon;
	for (int i = 0; i < 28; i++) {
		enemy = Entity(float(-2.5 + (i % 7) * 0.5), float(2.0 - (i / 7 * 0.5)), 475.0f / 1024.0f, 242.0f / 1024.0f, 1.0f, 0.05f);
		enemy.sprite = baddragon;
		aliens.push_back(enemy);
	}

	int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Chunk *playershoot;
	Mix_Chunk *playermove;
	playershoot = Mix_LoadWAV("shoot.wav");
	playermove = Mix_LoadWAV("playermove.wav");
	Mix_Music *music;
	music = Mix_LoadMUS("music.mp3");


	while (!done) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;	
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (state == MENU) {
						state = GAME;
						Mix_PlayMusic(music, -1);
					}
					else {
						shoot = true;
						Mix_PlayChannel(-1, playershoot, 0);
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					moveL = true;
					Mix_PlayChannel(-1, playermove, 0);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					moveR = true;
					Mix_PlayChannel(-1, playermove, 0);
				}
			}
			if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shoot = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					moveL = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					moveR = false;
				}
			}

		}
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		lastaShot += elapsed;
		lastpShot += elapsed;

		if (playing) {
			Update(elapsed);
			Render();
		}
		
	}

	Mix_FreeChunk(playershoot);
	Mix_FreeChunk(playermove);
	Mix_FreeMusic(music);

	SDL_Quit();
	return 0;
}

