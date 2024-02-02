# Mesh To Particles

Maya api node for creating particles at mesh vertices. These particles are not emitted or simulated, rather the particle data is set directly by the input mesh. Any color sets on the mesh are converted to per-particle attributes which can be used for shading, driving instancers, etc.

The original purpose was for representing Bifrost point geometry natively in Maya. However it's made to be general purpose and may be useful for other applications.

<img src="https://github.com/HeydoubleU/MeshToParticles/assets/56705510/928641cb-4ce2-45a0-9972-51723a7b5b82" width="600">


# Install

1. Copy `MeshToParticles.mll` to:
```
<documents>/maya/<version>/plug-ins
```
Note: the plug-ins folder may not exist and need to be created

2. (Optional) Copy `MeshToParticlesUtils.py`:
```
<documents>/maya/<version>/scripts
```

3. (Optional) Copy contents of `Compounds` to your bifrost compounds directory.

4. Load via the plug-in manager

<img src="https://github.com/HeydoubleU/MeshToParticles/assets/56705510/9505e180-6409-4a3e-b43b-8b6cfa2dd7f4" width="400">
