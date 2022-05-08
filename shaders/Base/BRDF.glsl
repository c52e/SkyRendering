struct MaterialData {
	vec3 F0;
	vec3 diffuse;
	vec3 normal;
	float roughness;
};

MaterialData LoadMeterialData(sampler2D albedo_tex, sampler2D normal_tex, sampler2D orm_tex, vec2 uv) {
	vec3 albedo = texture(albedo_tex, uv).rgb;
	vec3 normal = texture(normal_tex, uv).rgb;
	vec3 orm = texture(orm_tex, uv).rgb;

	MaterialData data;
	float metallic = orm.b;
	data.F0 = mix(vec3(0.04), albedo, metallic);
	data.diffuse = albedo - albedo * metallic;
	data.normal = normal;
	data.roughness = orm.g;

	return data;
}

//--------------------------------------
// from UE4
float Pow5(float x) {
	float x2 = x * x;
	return x2 * x2 * x;
}

vec3 F_Schlick(float HdotV, vec3 F0) {
	return F0 + (1.0 - F0) * Pow5(1.0 - HdotV);
}

float D_GGX(float a, float NdotH) {
	float a2 = a * a;
	float d = (NdotH * a2 - NdotH) * NdotH + 1.0;
	return a2 / (PI * d * d);
}

float Vis_SmithJointApprox(float a, float NdotV, float NdotL) {
	float Vis_SmithV = NdotL * (NdotV * (1 - a) + a);
	float Vis_SmithL = NdotV * (NdotL * (1 - a) + a);
	return 0.5 / max(Vis_SmithV + Vis_SmithL, 1e-5); // Bloom»áµ¼ÖÂNaNÀ©É¢
}
//--------------------------------------

vec3 BRDF(float NdotL, float NdotV, float NdotH, float HdotV, MaterialData data) {
	float a = data.roughness * data.roughness;
	float D = D_GGX(a, NdotH);
	float Vis = Vis_SmithJointApprox(a, NdotV, NdotL);
	vec3 F = F_Schlick(HdotV, data.F0);

	vec3 specular = vec3(D * Vis);
	vec3 diffuse = INV_PI * data.diffuse;
	return mix(diffuse, specular, F);
}
