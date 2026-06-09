# Plan: Render a top hat with the LEDs as a glowing hatband (POSIX simulator)

## Context

The POSIX port opens a macOS Metal window that visualizes the controller's
30 WS2812 LEDs as a flat ring of cubes spinning in space
([renderer.cpp:184-218](controller/src/hal/runtime/ports/posix/renderer.cpp#L184-L218)).
We want the simulator to look like the real product: an LED strip wrapped
around a **top hat**. The goal is to (1) load an actual downloaded 3D top-hat
model and draw it in the scene, and (2) reposition the LEDs into a horizontal
band around the crown of that hat, so it reads as a glowing hatband.

Decisions confirmed with the user:
- **Geometry source:** load a real downloaded model (not procedural).
- **LED placement:** a horizontal band wrapping the crown.

## Current state (what we're building on)

- Renderer: Apple **Metal via metal-cpp**. All geometry today is hand-built
  vertex/index arrays — a single unit cube drawn `NUM_PIXELS` times with
  instanced rendering ([renderer.cpp:91-156](controller/src/hal/runtime/ports/posix/renderer.cpp#L91-L156),
  [renderer.cpp:277-279](controller/src/hal/runtime/ports/posix/renderer.cpp#L277-L279)).
- Vertex format is position + normal; the shader does simple Phong lighting and
  per-instance color ([triangle.metal:27-55](controller/src/hal/runtime/ports/posix/shaders/triangle.metal#L27-L55)).
  **No shader changes are needed** — the hat is just another mesh fed through
  the same pipeline.
- There is **no model-loading code** anywhere — this is the main thing we add.
- Camera looks down −Z, FOV 45°, scene centered at `{0,0,-10}`
  ([renderer.cpp:225-239](controller/src/hal/runtime/ports/posix/renderer.cpp#L225-L239)).
- Build embeds shader source into headers via `xxd -i` and a CMake custom
  command ([CMakeLists.txt:3-24](controller/src/hal/runtime/ports/posix/CMakeLists.txt#L3-L24)).

## Approach

### 1. Source the model
Download a permissively-licensed (CC0 / MIT) top-hat model and convert it to a
**triangulated OBJ with normals**. Good sources: Poly Pizza, Kenney, Quaternius,
or a Sketchfab CC0 asset; if it comes as GLB/FBX, export to OBJ from Blender
("Triangulate Faces" + "Write Normals"). Save it as:

```
controller/src/hal/runtime/ports/posix/assets/top_hat.obj
```

Record the source URL + license in a sibling `assets/CREDITS.md` (matches the
project's attribution habits). Keep it low-poly — a few thousand triangles is
plenty for a simulator.

### 2. Vendor a single-header OBJ loader (in-tree, no submodule)
Per CLAUDE.md ("new work goes in-tree; resist new submodules"), drop the MIT
single-header [`tiny_obj_loader.h`](https://github.com/tinyobjloader/tinyobjloader)
into:

```
controller/src/hal/runtime/ports/posix/third_party/tiny_obj_loader.h
```

`#define TINYOBJLOADER_IMPLEMENTATION` in exactly one new `.cpp`.

### 3. Embed the OBJ as bytes (consistent with the shader pattern)
Reuse the existing `xxd -i` mechanism so the asset is compiled into the binary
(no runtime cwd / file-path fragility). Generalize the custom-command loop in
[CMakeLists.txt](controller/src/hal/runtime/ports/posix/CMakeLists.txt#L3-L24)
to also process `assets/*` into headers (e.g. `assets/top_hat.obj.h` exposing a
`unsigned char[]` + length), or add a second analogous block. Parse it from
memory with `tinyobj::ObjReader::ParseFromString`.

### 4. New mesh loader (`mesh_loader.cpp` / `.h`)
A small free function that:
- Parses the embedded OBJ string with tinyobjloader (request triangulation).
- Flattens to `std::vector<shader_types::VertexData>` (position + normal) and a
  `std::vector<uint32_t>` index list. **Use 32-bit indices** for the hat — real
  models routinely exceed 65 535 vertices (the LED cubes keep their existing
  16-bit indices).
- Computes face normals if the OBJ lacks them.
- **Recenters and uniformly scales** the model to a known canonical size (e.g.
  fit into a unit-ish bounding box and record crown radius + crown-band height),
  so step 6 can place the LED band on the crown surface deterministically
  regardless of the model's native units.

Reuse `math::` helpers in [math.h](controller/src/hal/runtime/ports/posix/math.h)
for any matrix work.

### 5. Draw the hat (Renderer changes)
In [renderer.h](controller/src/hal/runtime/ports/posix/renderer.h) add members:
`_pHatVertexBuffer`, `_pHatIndexBuffer`, `_hatIndexCount`, and
`_pHatInstanceBuffer[kMaxFramesInFlight]` (a 1-element `InstanceData` buffer).

- In `buildBuffers()` (or a new `buildHatGeometry()`): load the mesh and create
  the vertex/index buffers (`ResourceStorageModeManaged`, same as today).
- Add a `getHatInstanceBuffer()` mirroring `getInstanceDataBuffers()`: one
  `InstanceData` whose transform is `fullObjectRot * orient * scale` (the same
  scene spin the LEDs use, so hat + band rotate together), `orient` rotates the
  model so the crown points **+Y (up)**, and a dark felt color
  (e.g. `{0.05, 0.05, 0.06, 1.0}`).
- In `draw()` ([renderer.cpp:277-279](controller/src/hal/runtime/ports/posix/renderer.cpp#L277-L279)),
  after the LED instanced draw, issue a **second `drawIndexedPrimitives`** with
  `instanceCount = 1`, `IndexType::IndexTypeUInt32`, binding the hat vertex
  buffer at index 0 and the hat instance buffer at index 1 (camera buffer at
  index 2 is shared). Same PSO, same depth-stencil state — no new pipeline.

### 6. Reposition the LEDs into a hatband
Rewrite the position block in `getInstanceDataBuffers()`
([renderer.cpp:184-198](controller/src/hal/runtime/ports/posix/renderer.cpp#L184-L198))
so LEDs wrap the crown horizontally instead of lying in the XY plane:

```
x = R_band * cos(theta);
z = R_band * sin(theta);   // was z = 0
y = y_band;                // band height on the crown
```

where `R_band` and `y_band` come from the canonical crown dimensions recorded in
step 4 (band radius slightly larger than the crown so cubes sit on the surface).
Drop the per-LED `_angle`-driven wobble (the `zrot`/`yrot` at
[renderer.cpp:190-191](controller/src/hal/runtime/ports/posix/renderer.cpp#L190-L191))
so the band stays rigid like a real strip; keep the overall `fullObjectRot` spin.
Optionally orient each LED cube to face radially outward (nice-to-have).
LED color sourcing from `LEDBuffer` is unchanged
([renderer.cpp:210-211](controller/src/hal/runtime/ports/posix/renderer.cpp#L210-L211)).

### 7. Build wiring
In [CMakeLists.txt](controller/src/hal/runtime/ports/posix/CMakeLists.txt):
- Add `mesh_loader.cpp` (and the tinyobjloader-implementation TU if separate) to
  `target_sources`.
- Add the asset-embedding custom command + its header to the `ShaderHeaders`-style
  dependency so it's generated before compile.
- Add `third_party/` to the interface include dirs.

## Files to create / modify

- **Create** `.../posix/assets/top_hat.obj` (+ `assets/CREDITS.md`)
- **Create** `.../posix/third_party/tiny_obj_loader.h`
- **Create** `.../posix/mesh_loader.{h,cpp}`
- **Modify** `.../posix/renderer.h` — hat buffer members + helper decl
- **Modify** `.../posix/renderer.cpp` — load hat, hat instance buffer, second
  draw call, LED band repositioning
- **Modify** `.../posix/CMakeLists.txt` — sources, asset embedding, include dir

No changes to `triangle.metal`, `shader_types.h`, or `math.{h,cpp}` are required
(though a small radial-facing helper in `math` is optional for the nice-to-have).

## Verification

1. Build: `./beatled.sh controller posix build` — must compile clean (watch for
   tinyobjloader warnings; it's a large header).
2. Run the simulator and visually confirm:
   - A recognizable top hat renders, upright, lit by the existing Phong light.
   - The 30 LEDs form a horizontal band hugging the crown and rotate with the hat.
   - Driving a program (beats) lights the band as before — color flow intact.
3. Regression: `./beatled.sh test pico` (firmware POSIX tests) still passes — the
   render path isn't unit-tested, but this confirms the build/link didn't break
   shared targets.
4. Sanity-check the embedded-asset size impact on the binary is reasonable; if the
   OBJ is large, decimate it in Blender.

## Risks / notes

- **Model orientation/scale is model-specific.** The `orient` rotation and
  canonical-fit in step 4 absorb this, but expect to tweak `R_band`/`y_band` once
  visually against the chosen model.
- **Index width:** hat uses UInt32, LEDs stay UInt16 — don't share the index
  buffer or index type between the two draw calls.
- **License hygiene:** only use a CC0/MIT/CC-BY model and record attribution.
