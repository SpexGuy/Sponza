#version 400
// We'll replace the "//" in following lines with "#d" to turn flags on and off.
//efine AMBIENT_TEX 1
//efine DIFFUSE_TEX 1
//efine SPECULAR_TEX 1
//efine TRANSPARENCY_TEX 1
//efine NORMAL_TANGENT_TEX 1
//efine NORMAL_COLOR 1
//efine TEX_COORD_COLOR 1

// light pos is always used
uniform vec3 lightPos;

#if defined(AMBIENT_TEX)
uniform sampler2D ambientTex;
uniform vec3 ambientFilter;
#endif
#if defined(DIFFUSE_TEX)
uniform sampler2D diffuseTex;
uniform vec3 diffuseFilter;
#endif
#if defined(SPECULAR_TEX)
uniform sampler2D specularTex;
uniform float shininess;
uniform vec3 camPosition;
#endif
#if defined(TRANSPARENCY_TEX)
uniform sampler2D alphaTex;
#endif
#if defined(NORMAL_TANGENT_TEX)
uniform sampler2D normalTex;
#endif

in vec3 f_position;
in vec3 f_normal;
in vec2 f_tex;
in vec3 f_tangent;
in vec3 f_bitangent;

out vec4 fragColor;

void main() {
    // need to reject masked pixels so they don't write the depth buffer
#if defined(TRANSPARENCY_TEX)
    float alpha = texture(alphaTex, f_tex).r;
    if (alpha < 0.5) discard;
#endif

    // Texture fetches
#if defined(AMBIENT_TEX)
    vec3 ambientColor = texture(ambientTex, f_tex).rgb;
#endif
#if defined(DIFFUSE_TEX)
    vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
#endif
#if defined(SPECULAR_TEX)
    vec3 specularColor = texture(specularTex, f_tex).rgb;
#endif
#if defined(NORMAL_TANGENT_TEX)
    vec3 tsNormal = texture(normalTex, f_tex).rgb;
#endif

    // Lighting
    vec3 color = vec3(0);
#if defined(AMBIENT_TEX)
    color += ambientColor * ambientFilter;
#endif
#if defined(DIFFUSE_TEX) || defined(SPECULAR_TEX) || defined(NORMAL_COLOR) || defined(TEX_COORD_COLOR)
    vec3 lightDir = normalize(lightPos - f_position);
    #if defined(NORMAL_TANGENT_TEX)
        tsNormal = tsNormal * 2 + -1;
        vec3 normal = normalize(
            f_tangent * tsNormal.r +
            f_bitangent * tsNormal.g +
            f_normal * tsNormal.b);
    #else
        vec3 normal = normalize(f_normal);
    #endif
#endif
#if defined(DIFFUSE_TEX)
    float kDiffuse = max(0, dot(lightDir, normal));
    color += diffuseColor * diffuseFilter * kDiffuse;
#endif
#if defined(SPECULAR_TEX)
    vec3 viewDir = normalize(camPosition-f_position);
    vec3 halfway = normalize(lightDir + viewDir);
    float specAngle = max(0, dot(halfway, normal));
    float kSpecular = pow(specAngle, shininess);
    color += specularColor * kSpecular;
#endif
#if defined(NORMAL_COLOR) || defined(TEX_COORD_COLOR)
    float light = dot(normal, lightDir) * 0.5 + 0.5;
    light = (1-light) * 0.2 + light;
#endif
#if defined(NORMAL_COLOR)
    color = abs(normal) * light;
#endif
#if defined(TEX_COORD_COLOR)
    vec2 tex = abs(f_tex);
    float blue2 = 1 - tex.x*tex.x - tex.y*tex.y;
    color = vec3(tex, sqrt(max(0, blue2)));
#endif
    fragColor = vec4(color, 1);
}
