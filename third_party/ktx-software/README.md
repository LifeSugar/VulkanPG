# KTX-Software attribution

The in-process texture cooker in `src/Import/KtxTextureCooker.cpp` adapts the
texture construction, mip population, Basis Universal encoding, Zstandard
supercompression, and file-writing sequence used by KhronosGroup/KTX-Software.

- Upstream: https://github.com/KhronosGroup/KTX-Software
- Upstream version reviewed: v4.4.2
- Referenced files: `tools/toktx/toktx.cc`, `utils/scapp.h`,
  `lib/vkformat_enum.h`
- Copyright: 2010-2020 The Khronos Group Inc. and KTX-Software contributors
- License: Apache License 2.0; see `LICENSE-APACHE-2.0.txt`

Modifications in this project:

- Removed command-line parsing and process-launch behavior.
- Limited the initial engine integration to one 2D RGBA8 source image.
- Uses stb_image for PNG/JPG decoding and stb_image_resize2 for selectable mip
  filtering and edge behavior.
- Exposes typed settings for the editor rather than inferring texture semantics.
- Writes KTX2 directly through libktx, then imports the persistent file through
  `KtxTextureImporter`.
