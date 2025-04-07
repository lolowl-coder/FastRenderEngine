cd /D C:/Projects/FRE/Data/Shaders
%VK_SDK_PATH%/Bin/glslangValidator.exe -o fog.vert.spv -V fog.vert
%VK_SDK_PATH%/Bin/glslangValidator.exe -o fog.frag.spv -V fog.frag
%VK_SDK_PATH%/Bin/glslangValidator.exe -o textured.vert.spv -V textured.vert
%VK_SDK_PATH%/Bin/glslangValidator.exe -o textured.frag.spv -V textured.frag
%VK_SDK_PATH%/Bin/glslangValidator.exe -o normalMap.vert.spv -V normalMap.vert
%VK_SDK_PATH%/Bin/glslangValidator.exe -o normalMap.frag.spv -V normalMap.frag
%VK_SDK_PATH%/Bin/glslangValidator.exe -o pbr.vert.spv -V pbr.vert
%VK_SDK_PATH%/Bin/glslangValidator.exe -o pbr.frag.spv -V pbr.frag
%VK_SDK_PATH%/Bin/glslangValidator.exe -o colored.vert.spv -V colored.vert
%VK_SDK_PATH%/Bin/glslangValidator.exe -o colored.frag.spv -V colored.frag
rem pause