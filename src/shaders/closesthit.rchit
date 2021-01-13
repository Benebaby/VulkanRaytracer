/* Copyright (c) 2019-2020, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 texture;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform CameraProperties {mat4 view; mat4 proj;} cam;
layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;


void main()
{
  vec4 lightPos = vec4(100, 100, 0, 1);

  uint i0 = indices.i[3 * gl_PrimitiveID];
  uint i1 = indices.i[3 * gl_PrimitiveID + 1];
  uint i2 = indices.i[3 * gl_PrimitiveID + 2];
  vec3 v0 = vertices.v[i0].pos;
  vec3 v1 = vertices.v[i1].pos;
  vec3 v2 = vertices.v[i2].pos;
  vec3 n0 = vertices.v[i0].normal;
  vec3 n1 = vertices.v[i1].normal;
  vec3 n2 = vertices.v[i2].normal;
  vec2 t0 = vertices.v[i0].texture;
  vec2 t1 = vertices.v[i1].texture;
  vec2 t2 = vertices.v[i2].texture;

  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 pos = vec3(v0 * barycentricCoords.x + v1 * barycentricCoords.y + v2 * barycentricCoords.z) / 3;
  vec3 normal = normalize(n0 * barycentricCoords.x + n1 * barycentricCoords.y + n2 * barycentricCoords.z);
  vec2 textureCoord = vec2(t0 * barycentricCoords.x + t1 * barycentricCoords.y + t2 * barycentricCoords.z) / 3;
  
  vec3 lightVector = normalize(lightPos.xyz - pos);
	float dot_product = max(dot(lightVector, normal), 0.2);
	hitValue = vec3(1.0) * dot_product;
}
