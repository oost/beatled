# Asset credits

## top_hat.obj

- **Model:** "Skibidi Mafia Hat (from the Skibid Mafia series)"
- **Author:** Gabs 92FG67
- **Source:** https://sketchfab.com/3d-models/skibidi-mafia-hat-from-the-skibid-mafia-series-d8a7fe05847944e49953a148fb2ba2db
- **License:** Creative Commons Attribution 4.0 (CC BY 4.0)
- **Required attribution:** "Skibidi Mafia Hat" by Gabs 92FG67, licensed under CC BY 4.0

### Processing

Downloaded as glTF-binary (`.glb`) and converted to a triangulated OBJ with
per-vertex normals. The simulator embeds this OBJ into the binary at build time
via `xxd -i` (see `CMakeLists.txt`) and parses it from memory with
tinyobjloader; nothing is read from disk at runtime. The OBJ's `mtllib` /
`usemtl` lines and texture coordinates are ignored — the hat is drawn as flat
felt, so only positions and normals are used.

The mesh is recentered and uniformly scaled to a canonical bounding box by the
mesh loader, so the original units do not matter (794 vertices / 1104
triangles).
