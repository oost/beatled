#include <tiny_obj_loader.h>

#include "mesh_loader.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

#include <simd/simd.h>

namespace mesh_loader {

namespace {
using simd::float3;

float3 readPosition(const tinyobj::attrib_t &attrib, int index) {
  return {attrib.vertices[3 * index + 0], attrib.vertices[3 * index + 1],
          attrib.vertices[3 * index + 2]};
}

float3 normalize(float3 v) {
  float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  if (len < 1e-8f) {
    return {0.f, 1.f, 0.f};
  }
  return {v.x / len, v.y / len, v.z / len};
}
} // namespace

LoadedMesh loadObjFromString(const std::string &obj_text) {
  tinyobj::ObjReaderConfig config;
  config.triangulate = true;
  config.vertex_color = false;

  tinyobj::ObjReader reader;
  if (!reader.ParseFromString(obj_text, /* mtl_text */ "", config)) {
    __builtin_printf("mesh_loader: failed to parse OBJ: %s\n", reader.Error().c_str());
    assert(false);
  }
  if (!reader.Warning().empty()) {
    __builtin_printf("mesh_loader: %s", reader.Warning().c_str());
  }

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();

  LoadedMesh out;

  // Flatten every triangle corner into its own vertex (no dedup). Simulator
  // meshes are tiny, and this keeps the position/normal pairing trivial.
  for (const tinyobj::shape_t &shape : shapes) {
    const tinyobj::mesh_t &mesh = shape.mesh;
    size_t indexOffset = 0;
    for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
      // triangulate=true guarantees 3 corners per face.
      const int fv = mesh.num_face_vertices[f];
      assert(fv == 3);

      tinyobj::index_t idx[3] = {mesh.indices[indexOffset + 0], mesh.indices[indexOffset + 1],
                                 mesh.indices[indexOffset + 2]};
      indexOffset += fv;

      float3 pos[3];
      for (int c = 0; c < 3; ++c) {
        pos[c] = readPosition(attrib, idx[c].vertex_index);
      }

      // Synthesise a flat face normal if the OBJ omitted normals.
      float3 faceNormal = normalize(simd::cross(pos[1] - pos[0], pos[2] - pos[0]));

      for (int c = 0; c < 3; ++c) {
        float3 normal = faceNormal;
        if (idx[c].normal_index >= 0 && !attrib.normals.empty()) {
          normal = {attrib.normals[3 * idx[c].normal_index + 0],
                    attrib.normals[3 * idx[c].normal_index + 1],
                    attrib.normals[3 * idx[c].normal_index + 2]};
        }
        out.vertices.push_back({pos[c], normalize(normal)});
        out.indices.push_back(static_cast<uint32_t>(out.indices.size()));
      }
    }
  }

  assert(!out.vertices.empty());

  // Recenter on the bounding-box center and uniformly scale to a canonical box
  // (largest extent == 2), so downstream placement is unit-independent.
  float3 bmin = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max()};
  float3 bmax = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                 std::numeric_limits<float>::lowest()};
  for (const auto &v : out.vertices) {
    bmin = simd::min(bmin, v.position);
    bmax = simd::max(bmax, v.position);
  }
  float3 center = (bmin + bmax) * 0.5f;
  float3 extent = bmax - bmin;
  float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
  float scale = maxExtent > 1e-8f ? 2.0f / maxExtent : 1.0f;

  for (auto &v : out.vertices) {
    v.position = (v.position - center) * scale;
    // Normals are unaffected by translation + uniform positive scale.
  }

  // Locate a band hugging the crown: 70% up the height, and the widest radius
  // in a thin slab there (so the LEDs sit just on the crown surface).
  float ymin = std::numeric_limits<float>::max();
  float ymax = std::numeric_limits<float>::lowest();
  for (const auto &v : out.vertices) {
    ymin = std::min(ymin, v.position.y);
    ymax = std::max(ymax, v.position.y);
  }
  float height = ymax - ymin;
  out.bandY = ymin + 0.70f * height;

  float crownRadius = 0.f;
  float fallbackRadius = 0.f;
  for (const auto &v : out.vertices) {
    float r = std::sqrt(v.position.x * v.position.x + v.position.z * v.position.z);
    fallbackRadius = std::max(fallbackRadius, r);
    if (std::fabs(v.position.y - out.bandY) < 0.12f * height) {
      crownRadius = std::max(crownRadius, r);
    }
  }
  out.crownRadius = crownRadius > 1e-6f ? crownRadius : fallbackRadius;

  return out;
}

} // namespace mesh_loader
