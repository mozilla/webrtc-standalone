#include <stdio.h>
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#if defined(DARWIN_GL)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <SDL.h>
#define PROGRAM_NAME "WebRTC GL Player"

namespace render {
static const GLchar* vertexSource =
  "#version 120\n"
  "attribute vec2 position;"
  "attribute vec2 texcoord;"
  "varying vec2 varTexcoord;"
  "void main() {"
  "   gl_Position = vec4(position, 0.0, 1.0);"
  "   varTexcoord = texcoord;"
  "}";

static const GLchar *fragmentSource =
  "varying vec2 varTexcoord;\n"
  "uniform sampler2D texY;\n"
  "uniform sampler2D texU;\n"
  "uniform sampler2D texV;\n"
  "void main(void) {\n"
  "  float r,g,b,y,u,v;\n"
  "  y=texture2D(texY, varTexcoord).r;\n"
  "  u=texture2D(texU, varTexcoord).r;\n"
  "  v=texture2D(texV, varTexcoord).r;\n"
  "  y=1.1643*(y-0.0625);\n"
  "  u=u-0.5;\n"
  "  v=v-0.5;\n"
  "  r=y+1.5958*v;\n"
  "  g=y-0.39173*u-0.81290*v;\n"
  "  b=y+2.017*u;\n"
  "  gl_FragColor=vec4(r,g,b,1.0);\n"
  "}\n";

static SDL_Window *mainwindow;
static SDL_GLContext maincontext;
static GLuint textureY;
static GLuint textureU;
static GLuint textureV;
static GLuint vertexShader;
static GLuint fragmentShader;
static GLuint shaderProgram;

static GLfloat vertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f
};

static GLfloat texcoord[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
};

void sdldie(const char *msg)
{
    printf("%s: %s\n", msg, SDL_GetError());
    SDL_Quit();
    exit(1);
}

void checkSDLError(int line = -1)
{
  const char *error = SDL_GetError();
  if (*error != '\0') {
    fprintf(stderr, "SDL Error: %s\n", error);
    if (line != -1) {
      fprintf(stderr, " + line: %i\n", line);
    }
    SDL_ClearError();
  }
}

void
Initialize()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    sdldie("Unable to initialize SDL");
  }
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  mainwindow = SDL_CreateWindow(PROGRAM_NAME,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                640, 480,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (!mainwindow) {
    sdldie("Unable to create window");
  }

  checkSDLError(__LINE__);

  maincontext = SDL_GL_CreateContext(mainwindow);
  checkSDLError(__LINE__);

  SDL_GL_SetSwapInterval(1);

  glGenTextures(1, &textureY);
  glBindTexture(GL_TEXTURE_2D, textureY);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenTextures(1, &textureU);
  glBindTexture(GL_TEXTURE_2D, textureU);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenTextures(1, &textureV);
  glBindTexture(GL_TEXTURE_2D, textureV);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  GLint status;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[1024];
    glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
    fprintf(stderr, "Compiler error: %s\n", buffer);
  }

  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[1024];
    glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
    fprintf(stderr, "Compiler error: %s\n", buffer);
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(vertexShader, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[1024];
    glGetProgramInfoLog(vertexShader, 1024, NULL, buffer);
    fprintf(stderr, "Program error: %s\n", buffer);
  }

  glUseProgram(shaderProgram);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureY);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, textureU);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, textureV);

  GLint texY = glGetUniformLocation(shaderProgram, "texY");
  glUniform1i(texY, 0);
  GLint texU = glGetUniformLocation(shaderProgram, "texU");
  glUniform1i(texU, 1);
  GLint texV = glGetUniformLocation(shaderProgram, "texV");
  glUniform1i(texV, 2);

  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);
}

void
Draw(const unsigned char* aImage, int size, int aWidth, int aHeight)
{
  if (mainwindow) {

fprintf(stderr, "Got %d x %d size: %d\n", aWidth, aHeight, size);

    const unsigned char* chanY = aImage;
    glBindTexture(GL_TEXTURE_2D, textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, aWidth, aHeight, 0, GL_RED, GL_UNSIGNED_BYTE, chanY);

    const unsigned char* chanU = aImage + (aWidth * aHeight);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, aWidth / 2, aHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, chanU);

    const unsigned char* chanV = aImage + (aWidth * aHeight) + (aWidth * aHeight / 4);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, aWidth / 2, aHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, chanV);

    glClearColor ( 0.0, 0.0, 0.0, 1.0 );
    glClear ( GL_COLOR_BUFFER_BIT );
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    SDL_GL_SwapWindow(mainwindow);
  }
}

bool
KeepRunning()
{
  bool result = true;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          result = false;
        }
      break;
      case SDL_QUIT:
        result = false;
      break;
    }
  }

  return result;
}

void
Shutdown() {
  // delete textures here.
  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);

  SDL_GL_DeleteContext(maincontext);
  SDL_DestroyWindow(mainwindow);
  SDL_Quit();
}

} // namespace render
