#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec3 attribs;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 texture;
};

struct Sphere
{
  vec4 pad[2];
  vec3 center;
  float radius;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UBO {mat4 inverseView; mat4 inverseProj; vec4 light;} ubo;
layout(binding = 6, set = 0) buffer Spheres { Sphere s[]; } spheres;


void main()
{
  vec4 lightPos = ubo.light;

  vec3 color = vec3(1.0, 0.0, 0.0);

  Sphere sphere = spheres.s[gl_PrimitiveID];
  sphere.center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;

  vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 normal = normalize(position - sphere.center);
  vec2 textureCoord = vec2(0.0);
  
  vec3 lightVector;
  if(lightPos.w > 0.0){
    lightVector = normalize(lightPos.xyz - position);
  }else{
    lightVector = normalize(lightPos.xyz);
  }
	float dot_product = max(dot(lightVector, normal), 0.2);
	hitValue = color * dot_product;

  //only triangles faced towards the light are worth a shadow ray
  if(dot_product > 0){
    float tmin = 0.001;
    float tmax = 0;
    if(lightPos.w > 0.0){
      tmax = distance(lightPos.xyz, position);
    }else{
      tmax = 10000.0;
    }
    shadowed = true;  
    // tracing the ray until the first hit, dont call the hit shader only the miss shader, ignore transparent objects
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, position, tmin, lightVector, tmax, 2);
    if (shadowed) {
      hitValue = color * 0.2;
    }
  }
}
