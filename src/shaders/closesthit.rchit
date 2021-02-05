#version 460
#extension GL_EXT_ray_tracing : enable

struct Vertex
{
  vec3 pos;
  int matID;
  vec3 normal;
  vec2 texture;
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
layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) uniform sampler2D texSampler;


void main()
{
  uint i0 = indices.i[3 * gl_PrimitiveID];
  uint i1 = indices.i[3 * gl_PrimitiveID + 1];
  uint i2 = indices.i[3 * gl_PrimitiveID + 2];
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  vec3 position = vertices.v[i0].pos * barycentricCoords.x + vertices.v[i1].pos * barycentricCoords.y + vertices.v[i2].pos * barycentricCoords.z;
       position = (gl_ObjectToWorldEXT * vec4(position, 1.0)).xyz;
  vec3 normal = normalize(vertices.v[i0].normal * barycentricCoords.x + vertices.v[i1].normal * barycentricCoords.y + vertices.v[i2].normal * barycentricCoords.z);
  vec2 textureCoord = vertices.v[i0].texture * barycentricCoords.x + vertices.v[i1].texture * barycentricCoords.y + vertices.v[i2].texture * barycentricCoords.z;
  float reflectance = 0.0;
  vec3 color = vec3(1.0);
  if(vertices.v[i0].matID <= 8)
    color = vec3(vertices.v[i0].matID/8.0, 0.0, 0.0);
  else if(vertices.v[i0].matID <= 16)
    color = vec3(0.0, (vertices.v[i0].matID - 8)/8.0, 0.0);
  else
    color = vec3(0.0, 0.0, (vertices.v[i0].matID - 16)/8.0);
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
