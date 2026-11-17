# d3dx9: a stand-in for d3dx9_38.dll

Portable C++ for the 25 functions of `d3dx9_38.dll` the game imports (their declarations and thunks are in
`src/Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h/.cpp`, whose D3D9 types it uses: no Windows SDK header). The
Linux program compiles it by its folder; on Windows it is CMake option `FNVD_D3DX` (off: the program links
Microsoft's DLL until parity; on: the no-exe program links these objects in place of `d3dx9_38.lib`). Its float code
is never contracted into fused multiply-adds (`-ffp-contract=off` on its sources). It needs zlib's headers; the
program supplies zlib (`fnvd_zlib`).

## What it does

| Function | Stand-in |
| --- | --- |
| `D3DXMatrixMultiply`, `MatrixMultiplyTranspose`, `MatrixTranspose`, `Vec3TransformNormal`, `Vec4Transform`, `PlaneTransform` | the DLL's summation order: bit-equal |
| `Vec3Normalize`, `PlaneNormalize`, `Vec3TransformCoord` | within 3 ulp (the DLL uses SSE estimates) |
| `MatrixInverse` | the DLL's SSE inverse (Intel's, which its CPU dispatch picks) in the same order of operations: the determinant bit-equal, each element within 2 ulp (the DLL scales by a refined `rcpss` estimate of 1/det, CPU-dependent bits; the stand-in divides) |
| `MatrixRotationYawPitchRoll` | the C library's sin/cos; within 7e-7 of the output's largest value |
| `GetImageInfoFromFileInMemory`, `Create{,Cube,Volume}TextureFromFileInMemory` | DDS only (every game texture is DDS): the DLL's defaults (full mip chain, `D3DPOOL_MANAGED`, a format the device lacks becomes X8R8G8B8/A8R8G8B8); missing levels by the DLL's box filter. A file that needs resizing (not a power of two, larger than the device allows) or a format outside the table (`D3DX9Pixels.cpp`) fails with `D3DXERR_INVALIDDATA` and a line on stderr |
| `LoadSurfaceFromSurface` | whole surface to whole surface of one size (the game's one use): a copy, or decode and encode, with the DLL's ordered dither under `D3DX_DEFAULT`; anything else `E_NOTIMPL` and a stderr line. To DXT: its own encoder (below) |
| `SaveTextureToFileA` | PNG (the game's one call, `MapMenu`, asks for PNG), RGB or RGBA |
| `GetShaderConstantTable` | the shader's CTAB: `GetDesc`, `GetConstant`, `GetConstantByName`, `GetConstantDesc` as the DLL; the `Set*` methods return `D3DERR_INVALIDCALL` (the game never calls them) |
| `GetVertexShaderProfile`, `GetPixelShaderProfile` | the DLL's rules from the device caps |
| `CreateBuffer` | as the DLL |
| `CompileShader{,FromFileA}`, `AssembleShader{,FromFileA}` | fail with `E_NOTIMPL`, an error buffer and a stderr line naming the shader: no HLSL compiler. The game compiles only when its shader packages lack a shader |

Not the DLL's bytes by design:
- DXT levels the DLL makes (generated mips; files whose levels are narrower than 4 texels, which the DLL re-encodes
  and the stand-in copies from the file) come from the stand-in's encoder: principal axis, then least squares.
- `LoadSurfaceFromSurface` to DXT1 from a source with partial alpha: the DLL diffuses the alpha error under
  `D3DX_DEFAULT`; the stand-in thresholds at one half.
- Mips one texel wide or tall of uncompressed formats: the DLL rounds a few halves down (1 step).

## The comparison with the DLL

`test/D3DX9Compare.cpp` loads Microsoft's `d3dx9_38.dll` and compares it with the stand-in, linked in, on a Direct3D 9
device: math on random input, every DDS of the given BSAs, made-up DDS files of every supported format,
`LoadSurfaceFromSurface` both ways, `SaveTextureToFileA` read back by the DLL, the constant tables of every shader in
the given `.sdp` packages, the profiles on made-up caps.

Build (32-bit Windows, clang-cl): configure with `-DFNVD_NO_EXE=ON -DFNVD_D3DX=ON` and build `fnvd_d3dx9_compare`.
Run it under Wine with a Direct3D device (headless: `gamescope --backend headless -- wine ...`):

    D3DX9Compare <d3dx9_38.dll> <scratch dir> [-stride N] [-limit N] [-math N] [-wine dll] [-detail file.dds] \
        <file.bsa|file.sdp>...

It prints one line per difference (`FAIL ...`), per-class statistics and the failure count. The results measured
with the game's data are in the table above and below.

Measured (all 21 BSAs: 26,907 textures; all 16 `.sdp`; 20,000 math cases per function; 0 failures):
- math: multiply (also aliased), multiply-transpose, transpose, `Vec3TransformNormal`, `Vec4Transform`,
  `PlaneTransform` bit-equal; `Vec3Normalize`, `PlaneNormalize`, `Vec3TransformCoord` within 3 ulp (2.3e-7 of the
  output's largest); `MatrixRotationYawPitchRoll` within 6.6e-7 of the output's largest; `MatrixInverse` determinant
  bit-equal, elements within 2 ulp. Against an inverse in double precision the stand-in and the DLL err alike (mean
  2.2e-7 of the output's largest on general random matrices, max 1.2e-4 for both; affine 7.3e-8 / the DLL's 7.6e-8):
  the larger errors are the conditioning of random matrices, not the algorithm;
- textures, per class (format, kind): the levels the file holds are byte-equal in every class; the levels the DLL
  makes (mips, DXT levels under a block) are compared decoded, rms and max over the levels that differ, of 255:

  | Class | Files | File levels equal | Made levels: byte-equal | rms | max |
  | --- | --- | --- | --- | --- | --- |
  | DXT1 2D, mips | 19,178 | 121,438 of 121,438 | 17,398 of 39,497 | 1.51 | 46 |
  | DXT1 2D, one level | 253 | 253 of 253 | 80 of 2,449 | 2.64 | 132 |
  | DXT1 cube, mips | 6 | 144 of 144 | 11 of 72 | 1.21 | 10 |
  | DXT3 2D, mips | 843 | 6,102 of 6,102 | 902 of 1,891 | 1.69 | 14 |
  | DXT3 2D, one level | 643 | 643 of 643 | 567 of 4,919 | 2.18 | 126 |
  | DXT5 2D, mips | 4,658 | 34,777 of 34,777 | 1,836 of 10,053 | 1.49 | 52 |
  | DXT5 2D, one level | 1,297 | 1,297 of 1,297 | 234 of 10,273 | 3.13 | 193 |
  | A8R8G8B8 2D, mips | 8 | 72 of 72 | none made | | |
  | A8R8G8B8 2D, one level | 5 | 5 of 5 | 20 of 21 | 0.18 | 1 |
  | A4R4G4B4 2D, one level | 8 | 8 of 8 | 69 of 69 | | |
  | R8G8B8 2D, one level | 8 | (loaded as X8R8G8B8) | 0 of 11 | 0 | 0 (the X byte) |

  The DXT maxima are the stand-in's encoder against the DLL's on the made levels (by design, above). Made-up files
  (16 formats, 2D and volume, with and without mips): file levels byte-equal; made levels of 8-bit formats within
  2 of 255, of 4- and 5-bit formats within 17 (one or two steps of the format); A8 refused by both;
- `LoadSurfaceFromSurface` (every 50th texture): DXT -> A8R8G8B8 539 of 539 byte-equal; A8R8G8B8 -> DXT1/DXT3/DXT5
  on 538, decoded error against the source rms 71.6/1.43/0.99 (the DLL's 70.6/1.55/1.19), stand-in against DLL
  rms 29.0/1.77/1.32; one 2x8 surface is not encoded back (the device makes no DXT surface that is not whole blocks);
- `SaveTextureToFileA`: a DXT5 texture, random A8R8G8B8 and X8R8G8B8, a render target: read back by the DLL,
  difference 0;
- constant tables: 15,791 of 15,791 shaders equal (75,585 constants); profiles: the device's and 640 of 640 on
  made-up caps.
