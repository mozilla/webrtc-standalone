#include <stdio.h>
#include <stdlib.h>
/* If using gl3.h */
/* Ensure we are using opengl's core profile only */
#define GL3_PROTOTYPES 1
#include <OpenGL/gl3.h>

#include <SDL.h>
#define PROGRAM_NAME "WebRTC GL Player"

/* A simple function that prints a message, the error code returned by SDL,
 * and quits the application */
void sdldie(const char *msg)
{
    printf("%s: %s\n", msg, SDL_GetError());
    SDL_Quit();
    exit(1);
}


void checkSDLError(int line = -1)
{
    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        printf("SDL Error: %s\n", error);
        if (line != -1)
            printf(" + line: %i\n", line);
        SDL_ClearError();
    }
}

const GLchar* vertexSource =
    "#version 120\n"
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying vec2 varTexcoord;"
    "void main() {"
    "   gl_Position = vec4(position, 0.0, 1.0);"
    "   varTexcoord = texcoord;"
    "}";

const GLchar* fragmentSourceGrey =
    "#version 120\n"
    "varying vec2 varTexcoord;"
    "uniform sampler2D texY;"
    "uniform sampler2D texU;"
    "uniform sampler2D texV;"
    "void main() {"
    "   vec4 c = texture2D(texY, varTexcoord);" //  * vec4(1.0, 0.0, 0.0, 1.0);"
    "   gl_FragColor = vec4(c.r, c.r, c.r, 1.0);" //texture2D(texture, varTexcoord);" //  * vec4(1.0, 0.0, 0.0, 1.0);"
    "}";

const GLchar *fragmentSource=
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


/* Our program's entry point */
int main(int argc, char *argv[])
{
    SDL_Window *mainwindow; /* Our window handle */
    SDL_GLContext maincontext; /* Our opengl context handle */

    if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
        sdldie("Unable to initialize SDL"); /* Or die on error */

    /* Request opengl 3.2 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* Create our window centered at 512x512 resolution */
    mainwindow = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) /* Die if creation failed */
        sdldie("Unable to create window");

    checkSDLError(__LINE__);

    /* Create our opengl context and attach it to our window */
    maincontext = SDL_GL_CreateContext(mainwindow);
    checkSDLError(__LINE__);


    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(1);

//    GLuint vao;
//    glGenVertexArrays(1, &vao);
//    glBindVertexArray(vao);

    // Create a Vertex Buffer Object and copy the vertex data to it
//    GLuint vbo;
//    glGenBuffers(1, &vbo);

    GLfloat vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f
    };

    GLfloat texcoord[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

    const int YSize = 307200;
    const int UVSize = 76800;
    const int Width = 640;
    const int Height = 480;

    unsigned char chanY[YSize];
    unsigned char chanU[UVSize];
    unsigned char chanV[UVSize];

    FILE* file = fopen("frame.i420", "r");
    if (file) {
      fread(chanY, YSize, 1, file);
      fread(chanU, UVSize, 1, file);
      fread(chanV, UVSize, 1, file);
      fclose(file);
      file = 0;
    }
    else {
      fprintf(stderr, "Failed to open file\n");
      exit(1);
    }

    GLuint textureY;
    glGenTextures(1, &textureY);
    glBindTexture(GL_TEXTURE_2D, textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width, Height, 0, GL_RED, GL_UNSIGNED_BYTE, chanY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint textureU;
    glGenTextures(1, &textureU);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width / 2, Height /2, 0, GL_RED, GL_UNSIGNED_BYTE, chanU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint textureV;
    glGenTextures(1, &textureV);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width / 2, Height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, chanV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      char buffer[1024];
      glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
      fprintf(stderr, "Compiler error: %s\n", buffer);
    }

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      char buffer[1024];
      glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
      fprintf(stderr, "Compiler error: %s\n", buffer);
    }

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
//    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glGetProgramiv(vertexShader, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
      char buffer[1024];
      glGetProgramInfoLog(vertexShader, 1024, NULL, buffer);
      fprintf(stderr, "Program error: %s\n", buffer);
    }

    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);
    GLint texY = glGetUniformLocation(shaderProgram, "texY");
    glUniform1i(texY, /*GL_TEXTURE*/0);
    GLint texU = glGetUniformLocation(shaderProgram, "texU");
    glUniform1i(texU, /*GL_TEXTURE*/1);
    GLint texV = glGetUniformLocation(shaderProgram, "texV");
    glUniform1i(texV, /*GL_TEXTURE*/2);

    bool done = false;

    while (!done) {
      glClearColor ( 0.0, 0.0, 0.0, 1.0 );
      glClear ( GL_COLOR_BUFFER_BIT );
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      SDL_GL_SwapWindow(mainwindow);
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
              done = true;
            }
          break;
          case SDL_QUIT:
            done = true;;
          break;
        }
      }
    }

    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

//    glDeleteBuffers(1, &vbo);
//    glDeleteVertexArrays(1, &vao);

    /* Delete our opengl context, destroy our window, and shutdown SDL */
    SDL_GL_DeleteContext(maincontext);
    SDL_DestroyWindow(mainwindow);
    SDL_Quit();

    return 0;
}
