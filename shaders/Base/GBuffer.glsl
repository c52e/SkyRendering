#ifdef VERTEX
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
out vec3 vNormal;
out vec2 vUv;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform mat3 normal_matrix;
void main() {
	gl_Position = mvp * vec4(aPos, 1.0);
	vNormal = normalize(normal_matrix * aNormal);
	vUv = aUv;
}
#endif

#ifdef FRAGMENT
in vec3 vNormal;
in vec2 vUv;
layout(location = 0) out vec4 Albedo;
layout(location = 1) out vec4 Normal;
layout(location = 2) out vec4 ORM;
layout(location = 2) uniform vec3 albedo_factor;
layout(location = 3) uniform float metallic_factor;
layout(location = 4) uniform float roughness_factor;
layout(binding = 0) uniform sampler2D albedo_texture;
layout(binding = 1) uniform sampler2D orm_texture;
void main() {
	vec3 albedo = albedo_factor * pow(texture(albedo_texture, vUv).xyz, vec3(2.2));
	vec3 orm = texture(orm_texture, vUv).xyz;
	Albedo = vec4(albedo, 1.0);
	vec3 normal = normalize(vNormal);
	Normal = vec4(normal, 1.0);
	float metallic = metallic_factor * orm.b;
	float roughness = roughness_factor * orm.g;
	ORM = vec4(1.0, roughness, metallic, 1.0);
}
#endif
