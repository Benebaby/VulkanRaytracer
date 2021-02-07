#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct Sphere
{
  vec4 pad[2];
  vec3 center;
  float radius;
  int matID;
};

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 transmittance;
  vec3 emission;
  float shininess;
  float ior;                   // index of refraction
  float dissolve;              // 1 == opaque; 0 == fully transparent
  int illum;                   // Beleuchtungsmodell

  int ambientTexId;            // map_Ka
  int diffuseTexId;            // map_Kd
  int specularTexId;           // map_Ks
  int specularHighlightTexId;  // map_Ns
  int bumpTexId;               // map_bump, map_Bump, bump
  int displacementTexId;       // disp
  int alphaTexId;              // map_d
  int reflectionTexId;         // refl
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
layout(binding = 5, set = 0) uniform sampler2D texSampler[];
layout(binding = 6, set = 0) buffer Spheres { Sphere s[]; } spheres;
layout(binding = 7, set = 0) buffer Materials { Material m[]; } materials;


void main()
{
  Sphere sphere = spheres.s[gl_PrimitiveID];
  sphere.center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;

  vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 normal = normalize(position - sphere.center);
  vec3 rotatedNormal = mat3(gl_ObjectToWorldEXT) * normal;
  vec2 textureCoord = vec2((1 + atan(rotatedNormal.z, rotatedNormal.x) / 3.14159f) * 0.5, acos(rotatedNormal.y) / 3.14159f);
  Material material = materials.m[sphere.matID];
  
  vec3 color = vec3(1.0);
  if(material.diffuseTexId >= 0)
    color = texture(texSampler[material.diffuseTexId], textureCoord).xyz;
  else
    color = material.diffuse;

  float reflectance = 0.2;
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
  float cos_psi_n = pow(max(dot(lightVector, reflectedDir), 0.0f), material.shininess);
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
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, position, tmin, lightVector, tmax, 0);
    
  }

  if (Payload.shadow) {
    Payload.color += Payload.weight * (1.0 - reflectance) * 0.2 * color;
  }else{
    Payload.color += Payload.weight * (1.0 - reflectance) * (color * cos_phi + material.specular * cos_psi_n);
  }

  if(Payload.recursion < 4 && reflectance > 0.0001){
    Payload.weight *= reflectance;
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, position, 0.001, reflectedDir, 10000.0, 0);
  }
}
