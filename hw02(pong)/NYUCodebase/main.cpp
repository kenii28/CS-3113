/*
Kenneth Huynh
CS 3113 Game Programming
Hw02
Pong

*/


#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <iostream>
#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif



using namespace std;
SDL_Window* displayWindow;

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

struct Paddle
{
	Paddle(float top, float bottom, float left, float right) : top(top), bottom(bottom), left(left), right(right) {}

	float top;		
	float bottom;		
	float left;
	float right;
};

struct Ball
{
	Ball() {}

	float x;
	float y;
	float speed;
	float acc;
	float xVec;
	float yVec;
	
};

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;

	float lastFrameTicks = 0.0f;
	bool done = false;
	bool playing = false;

	glViewport(0, 0, 640, 360);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Paddle l(0.5f, -0.5f, -4.0f, -3.9f); // left paddle
	Paddle r(0.5f, -0.5f, 3.9f, 4.0f); // right paddle
	Ball b; // ball
	b.x = 0.0f;
	b.y = 0.0f;
	b.speed = 0.5f;
	b.acc = 2.0f;
	b.xVec = float(rand() % 10 + 2);
	b.yVec = float(rand() % 2 + 3);

	Matrix lPaddleMat;
	Matrix rPaddleMat;
	Matrix bMat;


	GLuint whiteTexture = LoadTexture(RESOURCE_FOLDER"white.png");
	
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);


	while (!done) {

		glClear(GL_COLOR_BUFFER_BIT);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;


		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;	
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && !playing) {
					playing = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_W && l.top < 2.0f) {
					l.top += 0.2f;
					l.bottom += 0.2f;
					lPaddleMat.Translate(0.0f, 0.2f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_S && l.bottom > -2.0f) {
					l.top -= 0.2f;
					l.bottom -= 0.2f;
					lPaddleMat.Translate(0.0f, -0.2f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_UP && r.top < 2.0f) {
					r.top += 0.2f;
					r.bottom += 0.2f;
					rPaddleMat.Translate(0.0f, 0.2f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && r.bottom > -2.0f ) {
					r.top -= 0.2f;
					r.bottom -= 0.2f;
					rPaddleMat.Translate(0.0f, -0.2f, 0.0f);
				}
			}

		}
		

		// LEFT PADDLE
		program.setModelMatrix(lPaddleMat);

		glBindTexture(GL_TEXTURE_2D, whiteTexture);

		float lVertices[] = { -4.0f, -0.5f, -3.9f, -0.5f, -3.9f, 0.5f, -3.9f, 0.5f, -4.0f, 0.5f, -4.0f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, lVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float lTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, lTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// RIGHT PADDLE
		program.setModelMatrix(rPaddleMat);

		
		glBindTexture(GL_TEXTURE_2D, whiteTexture);

		float rVertices[] = { 3.9f, -0.5f, 4.0f, -0.5f, 3.9f, 0.5f, 4.0f, 0.5f, 3.9f, 0.5f, 4.0f, -0.5f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float rTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, rTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.setModelMatrix(bMat);

		glBindTexture(GL_TEXTURE_2D, whiteTexture);

		float bVertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 0.1f, 0.1f, -0.1f, 0.1f, -0.1f, -0.1f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0,bVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float bTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, bTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		if (playing) {
			if (b.x <= l.right && b.y <= l.top && b.y >= l.bottom || b.x >= r.left && b.y <= r.top && b.y >= r.bottom){
				b.xVec *= -1;
				b.speed += (b.acc * elapsed);
				b.x += (b.speed * b.xVec * elapsed);
				b.y += (b.speed * b.yVec * elapsed);
				bMat.Translate((b.speed * b.xVec * elapsed), (b.speed * b.yVec * elapsed), 0.0f);
			}
			else if (b.y + 0.2f >= 2.0f || b.y - 0.2f <= -2.0f) {
				b.yVec *= -1;
				b.x += (b.speed * b.xVec * elapsed);
				b.y += (b.speed * b.yVec * elapsed);
				bMat.Translate((b.speed * elapsed), (b.speed * elapsed), 0.0f);
			}
			else if (b.x <= l.left || b.x >= r.right) {
				playing = false;
			}
			else {
				b.x += (b.speed * b.xVec * elapsed);
				b.y += (b.speed * b.yVec * elapsed);
				bMat.Translate((b.speed * b.xVec * elapsed), (b.speed * b.yVec * elapsed), 0.0f);
			}
		}

		SDL_GL_SwapWindow(displayWindow);

	}

	SDL_Quit();
	return 0;
}

