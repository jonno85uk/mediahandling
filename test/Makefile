CXX="g++"

CXX_FLAGS="-g3 -std=gnu++17 -Wall"
LIB_DIR="-L../build/debug"
LIBS="-lMediaHandling -lstdc++fs"
INCLUDES="-I../"

#all:
#	g++ -O0 -g3 -std=gnu++17 -Wall  main.cpp ${INCLUDES} -L ../build/debug/ ${LIBS}
all:
	g++ -O0 -g3 main.cpp -I ../ -I../Include -I../ffmpeg -std=gnu++17 -L ../build/debug/ -lMediaHandling -lstdc++fs
