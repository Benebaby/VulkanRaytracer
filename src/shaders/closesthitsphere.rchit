#version 460
#extension GL_EXT_ray_tracing : enable

struct Sphere
{
  vec4 pad[2];
  vec3 center;
  float radius;
  int matID;
};
struct RayPayload {
	vec3 color;
	bool shadow;
	int recursion;
  float weight;
};

layout(location = 0) rayPayloadInEXT RayPayload Payload;
hitAttributeEXT vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UBO {mat4 inverseView; mat4 inverseProj; vec4 light;} ubo;
layout(binding = 6, set = 0) buffer Spheres { Sphere s[]; } spheres;


void main()
{
  Sphere sphere = spheres.s[gl_PrimitiveID];
  sphere.center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;
  
  vec3 color = vec3(1.0);
  if(sphere.matID == 5)
    color = vec3(1.0, 0.0, 1.0);

  vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 normal = normalize(position - sphere.center);
  vec2 textureCoord = vec2(0.0);
  float reflectance = 0.5;
  Payload.recursion++; 
  
  vec4 lightPos = ubo.light;
  vec3 lightVector;
  if(lightPos.w > 0.0){
    lightVector = normalize(lightPos.xyz - position);
  }else{
    lightVector = normalize(lightPos.xyz);
  }
  vec3 reflectedDir = reflect(gl_WorldRayDirectionEXT, normal);
	float cos_phi = max(dot(lightVector, normal), 0.2);
  float cos_psi_n = pow(max(dot(lightVector, reflectedDir), 0.0f), 100.0);
  Payload.shadow = false;

  //only triangles faced towards the light are worth a shadow ray
  if(cos_phi > 0){
    float tmin = 0.001;
    float tmax = 0;
    if(lightPos.w > 0.0){
      tmax = distance(lightPos.xyz, position);
    }else{
      tmax = 10000.0;
    }
    Payload.shadow = true;  
    // tracing the ray until the first hit, dont call the hit shader only the miss shader, ignore transparent objects
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, position, tmin, lightVector, tmax, 0);
    
  }

  if (Payload.shadow) {
    Payload.color += Payload.weight * (1.0 - reflectance) * 0.2 * color;
  }else{
    Payload.color += Payload.weight * (1.0 - reflectance) * (color * cos_phi + vec3(1.0) * cos_psi_n);
  }

  if(Payload.recursion < 4 && reflectance > 0.0001){
    Payload.weight *= reflectance;
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, position, 0.001, reflectedDir, 10000.0, 0);
  }
}
