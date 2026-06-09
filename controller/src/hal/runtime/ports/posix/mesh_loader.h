#ifndef SRC__HAL__RUNTIME__PORTS__POSIX__MESH_LOADER__H_
#define SRC__HAL__RUNTIME__PORTS__POSIX__MESH_LOADER__H_

#include <cstdint>
#include <string>
#include <vector>

#include "shader_types.h"

namespace mesh_loader {

// A triangle mesh ready to feed through the same pipeline as the LED cubes:
// position + normal vertices and a 32-bit index list (models routinely exceed
// the 16-bit range, unlike the unit cube).
struct LoadedMesh {
  std::vector<shader_types::VertexData> vertices;
  std::vector<uint32_t> indices;

  // A horizontal band hugging the crown, in the same canonical units as the
  // recentered/scaled vertices below. Used to place the LED hatband on the
  // crown surface deterministically, independent of the model's native units.
  float crownRadius = 0.f;
  float bandY = 0.f;
};

// Parse a Wavefront OBJ from memory (triangulated; face normals synthesised
// when the OBJ lacks them). The mesh is recentered on its bounding-box center
// and uniformly scaled so its largest extent is 2 (a roughly unit-sized box),
// so callers can place and scale it deterministically.
LoadedMesh loadObjFromString(const std::string &obj_text);

} // namespace mesh_loader

#endif // SRC__HAL__RUNTIME__PORTS__POSIX__MESH_LOADER__H_
