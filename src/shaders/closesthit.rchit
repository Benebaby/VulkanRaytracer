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
  
  vec4 lightPos = ubo.light;
  vec3 lightVector;
  if(lightPos.w > 0.0){
    lightVector = normalize(lightPos.xyz - position);
  }else{
    lightVector = normalize(lightPos.xyz);
  }
	float dot_product = max(dot(lightVector, normal), 0.2);
	hitValue = texture(texSampler, textureCoord).xyz * dot_product;

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
      hitValue = texture(texSampler, textureCoord).xyz * 0.2;
    }
  }
}
