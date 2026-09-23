# Embedded distant boat

`boat.mesh` and `boat.tga` are the supplied `cj_dl_boat_imposter_high.mesh`
and `de_imp_boat_d.tga`, renamed for the converter. The original size is kept.
The model is rotated 180 degrees about Z and centered longitudinally so +Y
points toward the bow; source Z=0 is used as the waterline.

Run `python resources/distant-boat/generate.py` with Pillow installed to regenerate
`source/DistantBoatMesh.hpp`. The converter removes the coincident second hull,
fixes winding against the supplied normals, subdivides triangles once and samples
the texture into vertex colours. The generated header is compiled into the ASI;
these input assets and Python are not required by the game.
