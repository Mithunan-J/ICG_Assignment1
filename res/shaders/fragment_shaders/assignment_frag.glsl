#version 430

#include "../fragments/fs_common_inputs.glsl"

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

////////////////////////////////////////////////////////////////
/////////////// Instance Level Uniforms ////////////////////////
////////////////////////////////////////////////////////////////

// Represents a collection of attributes that would define a material
// For instance, you can think of this like material settings in 
// Unity
struct Material {
	sampler2D Diffuse;
	float     Shininess;
	sampler2D Specular;
	int choice;
};

struct InvaderMaterial {
	sampler2D blueDiffuse;
	sampler2D redDiffuse;
	sampler2D pinkDiffuse;
	sampler2D purpleDiffuse;
	sampler2D yellowDiffuse;
	sampler2D orangeDiffuse;
	int selection;
};
// Create a uniform for the material
uniform Material u_Material;
uniform InvaderMaterial u_InvaderMaterial;
uniform sampler1D s_Chilly;

////////////////////////////////////////////////////////////////
///////////// Application Level Uniforms ///////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/multiple_point_lights.glsl"

////////////////////////////////////////////////////////////////
/////////////// Frame Level Uniforms ///////////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/frame_uniforms.glsl"
#include "../fragments/color_correction.glsl"

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Normalize our input normal
	vec3 normal = normalize(inNormal);
	float specPower = texture(u_Material.Specular, inUV).r;
	// Use the lighting calculation that we included from our partial file
	//vec3 lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, specPower); //original
	
	//vec3 lightAccumulation = CalcAllLightContributionDiffuseOnly(inWorldPos, normal, u_CamPos.xyz);
	vec3 toEye = normalize(u_CamPos.xyz - inWorldPos);
	vec3 environmentDir = reflect(-toEye, normal);
	vec3 reflected = SampleEnvironmentMap(environmentDir);

	//viewDir
	vec3 viewDir = normalize(u_CamPos.xyz - inWorldPos);

	//Diffuse
	vec3 diffuseOut = CalcDiffuse(inWorldPos, normal, Lights[0]);

	//Specular
	vec3 specularOut = CalcSpecular(inWorldPos, normal, viewDir, Lights[0], 32);

	
	
	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(u_Material.Diffuse, inUV);

	
	vec3 result;
	vec3 lightAccumulation;
	

	switch(u_Material.choice) {
		case(3): //specular lighting only 
			lightAccumulation = CalcAllLightContributionSpecularOnly(inWorldPos, normal, u_CamPos.xyz, specPower);
			result = lightAccumulation  * inColor * textureColor.rgb;
			break;
		case(4): //ambient + specular //ambient set to 0.5 in testcomponent.cpp
			lightAccumulation = CalcAllLightContributionSpecularOnly(inWorldPos, normal, u_CamPos.xyz, specPower);
			result = lightAccumulation  * inColor * textureColor.rgb;
			break;
		case(5): //Ambient + specular + [your custom effect / shader]
			lightAccumulation = CalcAllLightContributionSpecularOnly(inWorldPos, normal, u_CamPos.xyz, specPower);
			float average = (textureColor.r + textureColor.g + textureColor.b)/3;
			vec4 newTexColor = vec4(average, average, average, 1.0);
			result = lightAccumulation  * inColor * newTexColor.rgb;
			break;
		case(6): //diffuse warp/ramp
			lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, specPower); //original
			result = lightAccumulation  * inColor * textureColor.rgb;
			result.r = texture(s_Chilly, result.r).r;
			result.g = texture(s_Chilly, result.g).g;
			result.b = texture(s_Chilly, result.b).b;
			break; 
		case(7): //specular warp/ramp
			specPower = texture(s_Chilly, inColor.r).r;
			lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, specPower); //original
			result = lightAccumulation  * inColor * textureColor.rgb;
			break;
		default: //diffuse and specular
			lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, specPower); //original
			result = lightAccumulation  * inColor * textureColor.rgb;
			break;
	}

	// combine for the final result
	//vec3 result = lightAccumulation  * inColor * test.rgb;
	
	//vec3 result = inColor * textureColor.rgb;
	//frag_color = vec4(ColorCorrect(result), test.a);
	//vec3 result = (diffuseOut + specularOut) * textureColor.rgb;
	//frag_color = vec4(ColorCorrect(mix(result, reflected, specPower)), textureColor.a);
	frag_color = vec4(ColorCorrect(result), textureColor.a);
}