echo off

set srcDir=%1
set outDir=%srcDir%/build/

echo Compilando shaders

if not exist %outDir% mkdir "%outDir%"

for %%f in (%srcDir%\*) do (
    if %%~xf == .vert %VULKAN_SDK%/Bin/glslangValidator -S vert -o %outDir%%%~nf -V %%f
    if %%~xf == .frag %VULKAN_SDK%/Bin/glslangValidator -S frag -o %outDir%%%~nf -V %%f
    if %%~xf == .comp %VULKAN_SDK%/Bin/glslangValidator -S comp -o %outDir%%%~nf -V %%f
)

echo Moviendo Shaders a los proyectos

for /D %%s in (%~dp0..\projects\*) do (

    if not exist %%s\shaders\build\ mkdir "%%s\shaders\build\"

    if not %%~ns == VulkanRenderEngine (
        copy "%outDir%" "%%s\shaders\build\"
    )
)