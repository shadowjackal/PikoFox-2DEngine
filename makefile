Linux :
	g++ main.cpp glad.c graphics.cpp -o Build/jackal -Bstatic -lglfw -lGL -lGLU -lm -lassimp -static-libstdc++ -static-libgcc -std=c++17
Windows :
	x86_64-w64-mingw32-g++ main.cpp glad.c graphics.cpp -o Build/jackal.exe -Bstatic -L -static -lglu32 -lwinmm -lopengl32 -mwindows -l:libglfw3.a -lgdi32 -l:libassimp.a -lminizip -lz -static-libstdc++ -static-libgcc -std=c++17  -Wl,--subsystem,windows
