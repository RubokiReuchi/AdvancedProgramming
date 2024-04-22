#ifdef RENDER_INDICATORS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec3 vPosition;

void main()
{
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vPosition;

layout(location = 0) out vec4 oColor;

void main()
{
    oColor = vec4(1.0);
}

#endif
#endif