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

struct Light
{
  vec4 pos;
  vec3 attenuation;
  float ambientIntensity;
  vec3 color;
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

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 5, set = 0) uniform sampler2D texSampler[];
layout(binding = 6, set = 0) buffer Spheres { Sphere s[]; } spheres[];
layout(binding = 7, set = 0) buffer Materials { Material m[]; } materials;
layout(binding = 8, set = 0) buffer Lights { Light l[]; } lights;

vec3 processDirLight(Light light, vec3 hitPos, vec3 normal, float shininess, vec3 ambi, vec3 diff, vec3 spec){
  vec3 lightVector = normalize(light.pos.xyz);
  float cos_phi = max(dot(lightVector, normal), 0.0);
  vec3 reflectedDir = reflect(gl_WorldRayDirectionEXT, normal);
  float cos_psi_n = pow(max(dot(lightVector, reflectedDir), 0.0), shininess);
  vec3 ambient  = light.ambientIntensity * light.color  * ambi;

  Payload.shadow = false; 
  if(cos_phi > 0){
    float tmin = 0.001;
    float tmax = 10000.0;
    Payload.shadow = true;  
    // tracing the ray until the first hit, dont call the hit shader only the miss shader, ignore transparent objects
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, hitPos, tmin, lightVector, tmax, 0);
  }
  if(Payload.shadow){
    return light.ambientIntensity * light.color  * ambi;
  }

  vec3 diffuse  = light.color  * cos_phi * diff;
  vec3 specular = light.color * cos_psi_n * spec;
  return (ambient + diffuse + specular);
}

vec3 processPointLight(Light light, vec3 hitPos, vec3 normal, float shininess, vec3 ambi, vec3 diff, vec3 spec){
  vec3 lightVector = normalize(light.pos.xyz - hitPos);
  float cos_phi = max(dot(lightVector, normal), 0.0);
  vec3 reflectedDir = reflect(gl_WorldRayDirectionEXT, normal);
  float cos_psi_n = pow(max(dot(lightVector, reflectedDir), 0.0), shininess);

  float distance = distance(light.pos.xyz, hitPos);
  float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance));
  vec3 ambient  = light.ambientIntensity * light.color  * ambi * attenuation;
  Payload.shadow = false; 
  if(cos_phi > 0){
    float tmin = 0.001;
    float tmax = distance;
    Payload.shadow = true;  
    // tracing the ray until the first hit, dont call the hit shader only the miss shader, ignore transparent objects
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, hitPos, tmin, lightVector, tmax, 0);
  }
  if(Payload.shadow){
    return light.ambientIntensity * light.color  * ambi * attenuation;
  }

  vec3 diffuse  = light.color  * cos_phi * diff * attenuation;
  vec3 specular = light.color * cos_psi_n * spec * attenuation;
  return (ambient + diffuse + specular);
}

vec3 refractRay(const vec3 I, const vec3 N, const float ior) { 
    float cosi = clamp(-1, 1, dot(I, N)); 
    float etai = 1, etat = ior; 
    vec3 n = N; 
    if (cosi < 0) { 
      cosi = -cosi; 
    } else {
      float oldetai = etai;
      etai = etat;
      etat = oldetai;
      n= -N; 
    }
    float eta = etai / etat; 
    float k = 1 - eta * eta * (1 - cosi * cosi); 
    if(k < 0)  
      return vec3(0);
    else 
      return eta * I + (eta * cosi - sqrt(k)) * n;
} 

void fresnel(const vec3 I, const vec3 N, const float ior, const float dissolve, inout float kr, inout float kt){ 
    float cosi = clamp(-1, 1, dot(I, N)); 
    float etai = 1; 
    float etat = ior; 
    if (cosi > 0) { 
      float oldetai = etai;
      etai = etat;
      etat = oldetai;
    } 
    // Compute sini using Snell's law
    float sint = etai / etat * sqrt(max(0.f, 1 - cosi * cosi)); 
    // Total internal reflection
    if (sint >= 1) { 
        kr = 1; 
    } 
    else { 
        float cost = sqrt(max(0.f, 1 - sint * sint)); 
        cosi = abs(cosi); 
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost)); 
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost)); 
        kr = (Rs * Rs + Rp * Rp) / 2; 
    } 
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
    kt = 1.0 - kr;
    kt *= Payload.weight * 0.95;
    kr *= Payload.weight * 0.95;
} 

void main()
{
  Sphere sphere = spheres[gl_InstanceCustomIndexEXT].s[gl_PrimitiveID];
  sphere.center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;

  vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
       position = sphere.center + sphere.radius * normalize(position - sphere.center);
  vec3 normal = normalize(position - sphere.center);
  vec3 rotatedNormal = mat3(gl_ObjectToWorldEXT) * normal;
  vec2 textureCoord = vec2((1 + atan(rotatedNormal.z, rotatedNormal.x) / 3.14159f) * 0.5, acos(rotatedNormal.y) / 3.14159f);
  Material material = materials.m[sphere.matID];
  
  vec3 diffuse = vec3(1.0);
  if(material.diffuseTexId >= 0)
    diffuse = texture(texSampler[material.diffuseTexId], textureCoord).xyz;
  else
    diffuse = material.diffuse;

  vec3 specular = vec3(1.0);
  if(material.specularTexId >= 0)
    specular = texture(texSampler[material.specularTexId], textureCoord).xyz * material.specular;
  else  
    specular = material.specular;

  vec3 ambient = vec3(1.0);
  if(material.ambientTexId >= 0){
    ambient = texture(texSampler[material.ambientTexId], textureCoord).r * diffuse;
  }else{
    ambient = diffuse;
  }  

  float reflectance = 0.0;
  if(material.illum == 3)
    reflectance = 1.0 - material.dissolve;
  
  float refractance = 0.0;
  if(material.illum == 7){
    fresnel(gl_WorldRayDirectionEXT, normal, material.ior, material.dissolve, reflectance, refractance);
  }

  Payload.recursion++; 

  vec3 calculatedColor = vec3(0.0);
  uint NUM_OF_LIGHTS = 7;
  for(int i = 0; i < NUM_OF_LIGHTS; i++){
    float lightType = lights.l[i].pos.w;
    if(lightType > 0.0001){
      calculatedColor += processPointLight(lights.l[i], position, normal, material.shininess, ambient, diffuse, specular);
    }else{
      calculatedColor += processDirLight(lights.l[i], position, normal, material.shininess, ambient, diffuse, specular);
    }
  }

  Payload.color += Payload.weight * (1.0 - reflectance - refractance) * calculatedColor;
  
  if(Payload.recursion < 4){
    float parentWeight = Payload.weight;
    if(reflectance > 0.0001){
      Payload.weight = parentWeight * reflectance;
      traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, position, 0.001, reflect(gl_WorldRayDirectionEXT, normal), 10000.0, 0);
    }
    if(refractance > 0.0001){
      Payload.weight = parentWeight * refractance;
      traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, position, 0.01, refractRay(gl_WorldRayDirectionEXT, normal, material.ior), 10000.0, 0);
    }
  }
}
