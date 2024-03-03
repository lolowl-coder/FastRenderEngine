for %%f in (*.vert *.frag *.comp) do (
    glslc -o "%%~nxf.spv" "%%f"
)