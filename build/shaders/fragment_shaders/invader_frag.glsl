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

	// Use the lighting calculation that we included from our partial file
	vec3 lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, u_Material.Shininess);
	vec4 test;
	switch(u_InvaderMaterial.selection) {
		case(1):
			test = texture(u_InvaderMaterial.blueDiffuse, inUV);
			break;
		case(2):
			test = texture(u_InvaderMaterial.redDiffuse, inUV);
			break;
		case(3):
			test = texture(u_InvaderMaterial.pinkDiffuse, inUV);
			break;
		case(4):
			test = texture(u_InvaderMaterial.purpleDiffuse, inUV);
			break;
		case(5):
			test = texture(u_InvaderMaterial.yellowDiffuse, inUV);
			break;
		case(6):
			test = texture(u_InvaderMaterial.orangeDiffuse, inUV);
			break;
		default:
			break;
	}



	
	
	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(u_Material.Diffuse, inUV);

	// combine for the final result
	vec3 result = lightAccumulation  * inColor * test.rgb;

	frag_color = vec4(ColorCorrect(result), test.a);
}