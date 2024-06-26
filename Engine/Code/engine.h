//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "BufferSupFunctions.h"
#include "Globals.h"
#include "Camera.h"

const VertexV3V2 vertices[] = {
    {glm::vec3(-1.0,-1.0,0.0), glm::vec2(0.0,0.0)},
    {glm::vec3(1.0,-1.0,0.0), glm::vec2(1.0,0.0)},
    {glm::vec3(1.0,1.0,0.0), glm::vec2(1.0,1.0)},
    {glm::vec3(-1.0,1.0,0.0), glm::vec2(0.0,1.0)},
};

const u16 indices[] = {
    0,1,2,
    0,2,3
};

struct App
{
    void UpdateEntityBuffer();
    void UpdateIndicatorsBuffer();

    void ConfigureFrameBuffer(FrameBuffer& configFB);

    void RenderGeometry(const Program& bindedProgram);
    void RenderIndicatorsGeometry();

    const GLuint CreateTexture(const bool isFloatingPoint = false);

    // Camera
    Camera camera;
    void Rotate(float pitch, float roll, float yaw, vec3& dir);

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material>  materials;
    std::vector<Mesh>  meshes;
    std::vector<Model>  models;
    std::vector<Program>  programs;

    // program indices
    GLuint renderToBackBufferShader;

    GLuint renderToFrameBufferShader;
    GLuint frameBufferToQuadShader;

    GLuint renderIndicatorsShader;

    GLuint texturedMeshProgram_uTexture;

    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::string openglDebugInfo;

    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    Buffer localUniformBuffer;
    std::vector<Entity> entities;
    std::vector<Light> lights;
    std::vector<Entity> lightsIndicators;

    GLint globalParamsOffset;
    GLint globalParamsSize;

    FrameBuffer deferredFrameBuffer;

    int shownTextureIndex = 0;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);