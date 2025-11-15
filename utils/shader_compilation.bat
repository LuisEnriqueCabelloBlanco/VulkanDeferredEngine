set srcDir=%1
set outDir=%2

for %%f in (%srcDir%shaders/*) do (
    if %%~xf == .vert glslangValidator -I%srcDir%includes -S vert -o %outDir%%%~nf -V %srcDir%shaders/%%f
    if %%~xf == .frag glslangValidator -I%srcDir%includes -S frag -o %outDir%%%~nf -V %srcDir%shaders/%%f
)
