# Embedded sedan

`sedan.json` contains the exact vertices, normals, materials, colours and indices
of the existing procedural sedan. It is not the supplied GTA IV impostor. The
model is unchanged: 844 vertices, 390 triangles, metres, +Y forward, +Z up,
and Z=0 at tyre contact.

Run `python resources/distant-car/generate.py` to regenerate
`source/DistantCarMesh.hpp`. No third-party Python packages are required.
The generated header is compiled into the ASI; the game needs no resource files.

Each vertex row is `[x, y, z, nx, ny, nz, material, RGB]`. Material IDs are
Paint=0, Glass=1, Rubber=2, Chrome=3, Headlight=4, Taillight=5. Indices form
triangle lists. The exporter retained nine significant decimal digits so
single-precision coordinates and normals round-trip unchanged.
