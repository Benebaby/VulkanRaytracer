#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
	vec3 color;
	bool shadow;
	int recursion;
	float weight;
};

layout(location = 0) rayPayloadInEXT RayPayload Payload;

void main()
{
	Payload.shadow = false;
}