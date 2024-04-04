#ifdef BASE_MODEL

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

struct Light
{
    uint type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0, std140) uniform GlobalsParam
{
    vec3 uCameraPosition;
    uint uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParam
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
    vTexCoord = aTexCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vNormal = vec3(uWorldMatrix * vec4(aNormal, 1.0));
    vViewDir = uCameraPosition - vPosition;
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
    uint type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0, std140) uniform GlobalsParam
{
    vec3 uCameraPosition;
    uint uLightCount;
    Light uLight[16];
};

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;
layout(location = 0) out vec4 oColor;

void main()
{
    vec4 textureColor = texture(uTexture, vTexCoord);
    vec4 finalColor = vec4(0.0);

    for (uint i = 0; i < uLightCount; ++i)
    {
        Light light = uLight[i];
        vec3  lightDir = normalize(light.direction);
        vec3 lightResult = vec3(0.0);

        if (uLight[i].type == 0) // directional
        {
            float ambientStrenght = 0.2;
            vec3 ambient = ambientStrenght * light.color;

            float diff = max(dot(vNormal, lightDir), 0.0);
            vec3 diffuse  = diff * light.color;

            float specularStrenght = 0.1;
            vec3 reflectDir = reflect(-lightDir, vNormal);
            vec3 normalViewDir = normalize(vViewDir);
            float spec = pow(max(dot(normalViewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrenght * spec * light.color;

            lightResult = ambient + diffuse + specular;
            finalColor += vec4(lightResult, 1.0) * textureColor;
        }
        else // point
        {
            float constant = 1.0;
            float linear = 0.09;
            float quadratic = 0.032;
            float distance = length(light.position - vPosition);
            float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

            float ambientStrenght = 0.2;
            vec3 ambient = ambientStrenght * light.color;

            float diff = max(dot(vNormal, lightDir), 0.0);
            vec3 diffuse  = diff * light.color;

            float specularStrenght = 0.1;
            vec3 reflectDir = reflect(-lightDir, vNormal);
            vec3 normalViewDir = normalize(vViewDir);
            float spec = pow(max(dot(normalViewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrenght * spec * light.color;

            lightResult = ambient * attenuation + diffuse * attenuation + specular * attenuation;
            finalColor += vec4(lightResult, 1.0) * textureColor;
        }
    }

    oColor = finalColor;
}

#endif
#endif