cd deps
cmake -G "Visual Studio 17 2022" -A x64 -S glfw -B glfw-build -D BUILD_SHARED_LIBS=false -D GLFW_BUILD_TESTS=false -D GLFW_BUILD_DOCS=false -D GLFW_BUILD_EXAMPLES=false
SETLOCAL
Set currentDirectory=%CD%
cd "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin" 
MSBuild.exe %currentDirectory%\glfw-build\GLFW.sln /p:Configuration=Debug /p:Platform=x64
MSBuild.exe %currentDirectory%\glfw-build\GLFW.sln /p:Configuration=Release /p:Platform=x64
ENDLOCAL