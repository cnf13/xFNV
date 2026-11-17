# steamless-host: unpacking the Steam exe on Linux

Steamless (3.1.0.5, the release zip from github.com/atom0s/Steamless) is a .NET Framework program. Rather than
running it under Wine, this host loads its unpacker plugins in .NET 10 on Linux. The result is
`env/exe/FalloutNV.exe`, md5 `6a5ca0850be4b3ff98b6c14fc886941e` (docs/ENVIRONMENT.md "The DRM-free exe").

```sh
source harness/env.sh                        # isolated HOME for dotnet and NuGet
export NUGET_PACKAGES=$FNVD_ENV/nuget
mkdir -p env/steamless && unzip <Steamless zip> -d env/steamless
# the assemblies are flagged 32-bit-required, which a 64-bit runtime refuses to load
harness/tools/clear32bit.py env/steamless/Steamless.CLI.exe env/steamless/Plugins/*.dll
dotnet build harness/tools/steamless-host/host.csproj -c Release --artifacts-path env/steamless-host/artifacts
mkdir -p env/steamless/work && cp game/FalloutNV.exe env/steamless/work/
dotnet env/steamless-host/artifacts/bin/host/release/host.dll env/steamless env/steamless/work/FalloutNV.exe
#   plugin: Steamless.Unpacker.Variant21.x86.Main ... result: True
mkdir -p env/exe && cp env/steamless/work/FalloutNV.exe.unpacked.exe env/exe/FalloutNV.exe
md5sum env/exe/FalloutNV.exe                 # 6a5ca0850be4b3ff98b6c14fc886941e
```
