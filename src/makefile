commondir = ../common/

all: final

final : main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c
	gcc -Wall -Wextra -o martin -I$(commondir) -I../common/Linux -DGL_GLEXT_PROTOTYPES main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm -lstdc++

debug : main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c
	gcc -g -O0 -Wall -Wextra -o martin -I$(commondir) -I../common/Linux -DGL_GLEXT_PROTOTYPES main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm -lstdc++

windows : main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Windows/MicroGlut.c $(commondir)Windows/glew.c
	g++ -Wall -o martin -I$(commondir) -I../common/Windows -DGL_GLEXT_PROTOTYPES main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Windows/MicroGlut.c $(commondir)Windows/glew.c -lopengl32 -lglu32 -lgdi32 -lstdc++

run: final
	./martin

gdb: debug
	gdb ./martin
