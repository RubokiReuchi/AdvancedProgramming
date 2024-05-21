//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include "ModelLoadingFunctions.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(vertexShaderDefine),
        (GLint)programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(fragmentShaderDefine),
        (GLint)programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    GLint attributeCount = 0;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    for (GLuint i = 0; i < attributeCount; i++)
    {
        GLsizei bufSize = 256;
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        GLchar name[256];
        glGetActiveAttrib(program.handle, i, ARRAY_COUNT(name), &length, &size, &type, name);

        u8 location = glGetAttribLocation(program.handle, name);
        program.shaderLayout.attributes.push_back(VertexShaderAttribute{ location, u8(size) });
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

GLuint FindVAO(Mesh& mesh, u32 subMeshIdx, const Program& program)
{
    GLuint returnValue = 0;

    SubMesh& subMesh = mesh.subMeshes[subMeshIdx];
    for (u32 i = 0; i < (u32)subMesh.vaos.size(); ++i)
    {
        if (subMesh.vaos[i].programHandle == program.handle)
        {
            returnValue = subMesh.vaos[i].handle;
            break;
        }
    }

    if (returnValue == 0)
    {
        glGenVertexArrays(1, &returnValue);
        glBindVertexArray(returnValue);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        auto& ShaderLayout = program.shaderLayout.attributes;
        for (auto shaderIt = ShaderLayout.cbegin(); shaderIt != ShaderLayout.cend(); ++shaderIt)
        {
            bool attributeWasLinked = false;
            auto& SubMeshLayout = subMesh.vertexBufferLayout.attributes;
            for (auto subMeshIt = SubMeshLayout.cbegin(); subMeshIt != SubMeshLayout.cend(); ++subMeshIt)
            {
                if (shaderIt->location == subMeshIt->location)
                {
                    const u32 index = subMeshIt->location;
                    const u32 ncomp = subMeshIt->componentCount;
                    const u32 offset = subMeshIt->offset + subMesh.vertexOffset;
                    const u32 stride = subMesh.vertexBufferLayout.stride;

                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }

            assert(attributeWasLinked);
        }

        glBindVertexArray(0);

        VAO vao = { returnValue, program.handle };
        subMesh.vaos.push_back(vao);
    }

    return returnValue;
}

glm::mat4 TranformScale(const vec3& scaleFactors)
{
    return glm::scale(scaleFactors);
}

glm::mat4 TransformPositionScale(const vec3& position, const vec3& scaleFactors)
{
    return scale(glm::translate(position), scaleFactors);
}

glm::mat4 RotateMatrix(const glm::mat4 matrix, const vec3& direction)
{
    return glm::rotate(matrix, 1.0f, direction);
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    // Get OpenGL Info
    app->openglDebugInfo += "OpenGL version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    // VBO
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // EBO
    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // VAO
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0); // layout 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)sizeof(glm::vec3)); // layout 1
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    app->camera.SetCamera(app->displaySize.x, app->displaySize.y, vec3(5, 5, 5));

    // Forward Mode
    app->renderToBackBufferShader = LoadProgram(app, "renderToBackBuffer.glsl", "RENDER_TO_BACK_BUFFER");

    // Deferred Mode
    app->renderToFrameBufferShader = LoadProgram(app, "renderToFrameBuffer.glsl", "RENDER_TO_FRAMEBUFFER");
    app->frameBufferToQuadShader = LoadProgram(app, "frameBufferToQuad.glsl", "FRAMEBUFFER_TO_QUAD");

    app->renderIndicatorsShader = LoadProgram(app, "shaders.glsl", "RENDER_INDICATORS");

    const Program& texturedGeometryProgram = app->programs[app->renderToBackBufferShader];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");
    u32 patrickModelIndex = ModelLoader::LoadModel(app, "Patrick/Patrick.obj");
    u32 groundModelIndex = ModelLoader::LoadModel(app, "Patrick/Ground.obj");

    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 3 * sizeof(float) });
    vertexBufferLayout.stride = 5 * sizeof(float);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->localUniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    app->entities.push_back({ TransformPositionScale(vec3(2.0, 0.0, -4.0), vec3(1.0, 1.0, 1.0)), patrickModelIndex, 0, 0 });
    app->entities.push_back({ TransformPositionScale(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), patrickModelIndex, 0, 0 });
    app->entities.push_back({ TransformPositionScale(vec3(-2.0, 0.0, 4.0), vec3(1.0, 1.0, 1.0)), patrickModelIndex, 0, 0 });

    app->entities.push_back({ TransformPositionScale(vec3(0.0, -5.0, 0.0), vec3(1.0, 1.0, 1.0)), groundModelIndex, 0, 0});

    app->lights.push_back({ LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(-1.0, -1.0, 0.0), vec3(0.0, 3.0, 0.0) });
    app->lights.push_back({ LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, 5.0, 0.0) });
    app->lights.push_back({ LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 7.0, 0.0) });

    app->lights.push_back({ LightType::LightType_Point, vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(-7.0, 1.0, -2.0) });
    app->lights.push_back({ LightType::LightType_Point, vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 2.0, -1.0) });
    app->lights.push_back({ LightType::LightType_Point, vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(3.0, 3.0, 5.0) });

    app->ConfigureFrameBuffer(app->deferredFrameBuffer);

    app->mode = Mode_Deferred;

    // lights indicators
    u32 squareModelIndex = ModelLoader::LoadModel(app, "Patrick/Quad.obj");
    u32 sphereModelIndex = ModelLoader::LoadModel(app, "Patrick/Sphere.obj");

    for (size_t i = 0; i < app->lights.size(); ++i)
    {
        u32 indicatorModel = (app->lights[i].type == LightType::LightType_Directional) ? squareModelIndex : sphereModelIndex;
        app->lightsIndicators.push_back({ TransformPositionScale(app->lights[i].position, vec3(0.3, 0.3, 0.3)), indicatorModel, 0, 0 });
        app->lightsIndicators[i].worldMatrix = RotateMatrix(app->lightsIndicators[i].worldMatrix, app->lights[i].direction);
    }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    ImGui::Text("%s", app->openglDebugInfo.c_str());

    const char* renderModes[] = { "Forward", "Deferred" };
    if (ImGui::BeginCombo("Render Mode", renderModes[app->mode]))
    {
        for (size_t i = 0; i < ARRAY_COUNT(renderModes); i++)
        {
            bool isSelected = (i == app->mode);
            if (ImGui::Selectable(renderModes[i], isSelected))
            {
                app->mode = static_cast<Mode>(i);
            }
        }
        ImGui::EndCombo();
    }

    
    if (app->mode == Mode_Deferred)
    {
        const char* colorAttachments[] = { "Albedo", "Normals", "Position", "ViewDir", "Depth" };
        if (ImGui::BeginCombo("Color Attachment", colorAttachments[app->shownTextureIndex]))
        {
            for (int n = 0; n < ARRAY_COUNT(colorAttachments); n++)
            {
                bool is_selected = (app->shownTextureIndex == n);
                if (ImGui::Selectable(colorAttachments[n], is_selected))
                {
                    app->shownTextureIndex = n;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Image(ImTextureID(app->deferredFrameBuffer.colorAttachments[app->shownTextureIndex]), ImVec2(250, 150), ImVec2(0, 1), ImVec2(1, 0));
    }

    ImGui::End();
}

void Update(App* app)
{
    if (app->input.mouseButtons[1] == BUTTON_PRESSED) // Mouse left
    {
        if (app->input.keys[33] == BUTTON_PRESSED) // W
        {
            app->camera.position += app->camera.speed * glm::normalize(app->camera.direction);
        }
        else if (app->input.keys[29] == BUTTON_PRESSED) // S
        {
            app->camera.position -= app->camera.speed * glm::normalize(app->camera.direction);
        }
        else if (app->input.keys[14] == BUTTON_PRESSED) // D
        {
            app->camera.position += app->camera.speed * glm::normalize(glm::cross(app->camera.direction, app->camera.up));
        }
        else if (app->input.keys[11] == BUTTON_PRESSED) // A
        {
            app->camera.position -= app->camera.speed * glm::normalize(glm::cross(app->camera.direction, app->camera.up));
        }
        else if (app->input.keys[15] == BUTTON_PRESSED) // E
        {
            app->camera.position += app->camera.speed * glm::normalize(app->camera.up);
        }
        else if (app->input.keys[27] == BUTTON_PRESSED) // Q
        {
            app->camera.position -= app->camera.speed * glm::normalize(app->camera.up);
        }
        if (app->input.mouseDelta != vec2(0, 0))
        {
            app->Rotate(-glm::radians(app->input.mouseDelta.x), 0, 0, app->camera.direction);
        }
    }
}

void App::Rotate(float pitch, float roll, float yaw, vec3& dir) {
    float cosa = cos(yaw);
    float sina = sin(yaw);

    float cosb = cos(pitch);
    float sinb = sin(pitch);

    float cosc = cos(roll);
    float sinc = sin(roll);

    float Axx = cosa * cosb;
    float Axy = cosa * sinb * sinc - sina * cosc;
    float Axz = cosa * sinb * cosc + sina * sinc;

    float Ayx = sina * cosb;
    float Ayy = sina * sinb * sinc + cosa * cosc;
    float Ayz = sina * sinb * cosc - cosa * sinc;

    float Azx = -sinb;
    float Azy = cosb * sinc;
    float Azz = cosb * cosc;

    float px = dir.x;
    float py = dir.y;
    float pz = dir.z;

    dir.x = Axx * px + Axy * py + Axz * pz;
    dir.y = Ayx * px + Ayy * py + Ayz * pz;
    dir.z = Azx * px + Azy * py + Azz * pz;
}

void Render(App* app)
{
    switch (app->mode)
    {
    case Mode_Forward:
    {
        app->UpdateEntityBuffer();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        const Program& forwardProgram = app->programs[app->renderToBackBufferShader];
        glUseProgram(forwardProgram.handle);

        app->RenderGeometry(forwardProgram);

        glUseProgram(0);
    }
    break;
    case Mode_Deferred:
    {
        app->UpdateEntityBuffer();

        // Render to FrameBuffer colorAttachments
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        glBindFramebuffer(GL_FRAMEBUFFER, app->deferredFrameBuffer.fbHandle);

        glDrawBuffers(app->deferredFrameBuffer.colorAttachments.size(), app->deferredFrameBuffer.colorAttachments.data());

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& deferredProgram = app->programs[app->renderToFrameBufferShader];
        glUseProgram(deferredProgram.handle);

        app->RenderGeometry(deferredProgram);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render to BackBuffer from colorAttachments
        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        const Program& frameBufferToQuadProgram = app->programs[app->frameBufferToQuadShader];
        glUseProgram(frameBufferToQuadProgram.handle);

        // Render Quad
        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->localUniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->deferredFrameBuffer.colorAttachments[0]);
        glUniform1i(glGetUniformLocation(frameBufferToQuadProgram.handle, "uAlbedo"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->deferredFrameBuffer.colorAttachments[1]);
        glUniform1i(glGetUniformLocation(frameBufferToQuadProgram.handle, "uNormals"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->deferredFrameBuffer.colorAttachments[2]);
        glUniform1i(glGetUniformLocation(frameBufferToQuadProgram.handle, "uPosition"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->deferredFrameBuffer.colorAttachments[3]);
        glUniform1i(glGetUniformLocation(frameBufferToQuadProgram.handle, "uViewDir"), 3);

        glBindVertexArray(app->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
        
    }
    break;
    default:;
    }

    // lights indicators
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    app->UpdateIndicatorsBuffer();
    app->RenderIndicatorsGeometry();
}

void App::UpdateEntityBuffer()
{
    camera.UpdateCameraAspectRatio(displaySize.x, displaySize.y);
    camera.Matrix(60.0f, 0.1f, 1000.0f);

    BufferManager::MapBuffer(localUniformBuffer, GL_WRITE_ONLY);

    // Global Params
    globalParamsOffset = localUniformBuffer.head;
    PushVec3(localUniformBuffer, camera.position);
    PushUInt(localUniformBuffer, lights.size());

    for (size_t i = 0; i < lights.size(); ++i)
    {
        BufferManager::AlignHead(localUniformBuffer, sizeof(vec4));

        Light& light = lights[i];
        PushUInt(localUniformBuffer, light.type);
        PushVec3(localUniformBuffer, light.color);
        PushVec3(localUniformBuffer, light.direction);
        PushVec3(localUniformBuffer, light.position);
    }

    globalParamsSize = localUniformBuffer.head - globalParamsOffset;

    // Local Params
    u32 iterator = 0;
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        glm::mat4 world = it->worldMatrix;
        glm::mat4 WVP = camera.projection * camera.view * world;

        Buffer& localBuffer = localUniformBuffer;
        BufferManager::AlignHead(localBuffer, uniformBlockAlignment);
        it->localParamsOffset = localBuffer.head;
        PushMat4(localBuffer, world);
        PushMat4(localBuffer, WVP);
        it->localParamSize = localBuffer.head - it->localParamsOffset;
        iterator++;
    }
    BufferManager::UnmapBuffer(localUniformBuffer);
}

void App::UpdateIndicatorsBuffer()
{
    camera.UpdateCameraAspectRatio(displaySize.x, displaySize.y);
    camera.Matrix(60.0f, 0.1f, 1000.0f);

    BufferManager::MapBuffer(localUniformBuffer, GL_WRITE_ONLY);

    // Local Params
    u32 iterator = 0;
    for (auto it = lightsIndicators.begin(); it != lightsIndicators.end(); ++it)
    {
        glm::mat4 world = it->worldMatrix;
        glm::mat4 WVP = camera.projection * camera.view * world;

        Buffer& localBuffer = localUniformBuffer;
        BufferManager::AlignHead(localBuffer, uniformBlockAlignment);
        it->localParamsOffset = localBuffer.head;
        PushMat4(localBuffer, world);
        PushMat4(localBuffer, WVP);
        it->localParamSize = localBuffer.head - it->localParamsOffset;
        iterator++;
    }
    BufferManager::UnmapBuffer(localUniformBuffer);
}

void App::ConfigureFrameBuffer(FrameBuffer& configFB)
{
    // Framebuffer class
    configFB.colorAttachments.push_back(CreateTexture());
    configFB.colorAttachments.push_back(CreateTexture(true));
    configFB.colorAttachments.push_back(CreateTexture(true));
    configFB.colorAttachments.push_back(CreateTexture(true));
    configFB.colorAttachments.push_back(CreateTexture(true));

    glGenTextures(1, &configFB.depthHandle);
    glBindTexture(GL_TEXTURE_2D, configFB.depthHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displaySize.x, displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &configFB.fbHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, configFB.fbHandle);

    std::vector<GLuint> drawBuffers;
    for (size_t i = 0; i < configFB.colorAttachments.size(); i++)
    {
        GLuint position = GL_COLOR_ATTACHMENT0 + i;
        glFramebufferTexture(GL_FRAMEBUFFER, position, configFB.colorAttachments[i], 0);
        drawBuffers.push_back(position);
    }
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, configFB.depthHandle, 0);

    glDrawBuffers(drawBuffers.size(), drawBuffers.data());

    GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        int y = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void App::RenderGeometry(const Program& bindedProgram)
{
    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), localUniformBuffer.handle, globalParamsOffset, globalParamsSize);
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), localUniformBuffer.handle, it->localParamsOffset, it->localParamSize);

        Model& model = models[it->modelIndex];
        Mesh& mesh = meshes[model.meshIdx];

        for (u32 i = 0; i < mesh.subMeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, bindedProgram);
            glBindVertexArray(vao);

            u32 subMeshMaterialIdx = model.materialIdx[i];
            Material subMeshMaterial = materials[subMeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.albedoTextureIdx].handle);
            glUniform1i(texturedMeshProgram_uTexture, 0);

            SubMesh& subMesh = mesh.subMeshes[i];
            glDrawElements(GL_TRIANGLES, subMesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)subMesh.indexOffset);
        }
    }
}


void App::RenderIndicatorsGeometry()
{
    const Program& program = programs[renderIndicatorsShader];
    glUseProgram(program.handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), localUniformBuffer.handle, globalParamsOffset, globalParamsSize);
    for (auto it = lightsIndicators.begin(); it != lightsIndicators.end(); ++it)
    {
        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), localUniformBuffer.handle, it->localParamsOffset, it->localParamSize);

        Model& model = models[it->modelIndex];
        Mesh& mesh = meshes[model.meshIdx];

        for (u32 i = 0; i < mesh.subMeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, program);
            glBindVertexArray(vao);

            SubMesh& subMesh = mesh.subMeshes[i];
            glDrawElements(GL_TRIANGLES, subMesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)subMesh.indexOffset);
        }
    }
    glUseProgram(0);
}

const GLuint App::CreateTexture(const bool isFloatingPoint)
{
    GLuint textureHandle;

    GLenum internalFormat = isFloatingPoint ? GL_RGBA16F : GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum dataType = isFloatingPoint ? GL_FLOAT : GL_UNSIGNED_BYTE;

    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, displaySize.x, displaySize.y, 0, format, dataType, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureHandle;
}