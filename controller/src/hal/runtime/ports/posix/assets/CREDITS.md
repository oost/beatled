# Asset credits

## top_hat.obj

- **Model:** "Top Hat"
- **Author:** Poly by Google
- **Source:** https://poly.pizza/m/1PP65IsuNwv
  (originally from the Google Poly archive)
- **Asset CDN:** https://static.poly.pizza/8b57048e-81b3-49cb-b6d9-6b23ace1a92c.glb
- **License:** Creative Commons Attribution 3.0 (CC BY 3.0)
- **Required attribution:** "Poly by Google"

### Processing

Downloaded as glTF-binary (`.glb`) and converted to a triangulated OBJ with
per-vertex normals (`f v//vn`). The simulator embeds this OBJ into the binary at
build time via `xxd -i` (see `CMakeLists.txt`) and parses it from memory with
tinyobjloader; nothing is read from disk at runtime.

The mesh is recentered and uniformly scaled to a canonical bounding box by the
mesh loader, so the original units do not matter (233 vertices / 240 triangles).
