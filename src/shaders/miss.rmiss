#version 460
#extension GL_EXT_ray_tracing : enable

struct RayPayload {
	vec3 color;
	bool shadow;
	int recursion;
    float weight;
};

layout(location = 0) rayPayloadInEXT RayPayload Payload;

void main()
{
    Payload.color += (Payload.weight) * vec3(0.0);
}