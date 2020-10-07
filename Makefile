test.e: test.cc gl_obj.h
	g++ -std=c++2a -lGL test.cc -o test.e -O2 -s

run: test.e
	./test.e

python: gl_renderer.cc
	g++ -shared -Wl,--export-dynamic gl_renderer.cc -fPIC -lboost_python38 -lpython3.8 -I /usr/include/python3.8 -I ${VIRTUAL_ENV}/include -lGL `pkg-config --cflags --libs gtk+-3.0` -o gl_renderer.so -std=c++2a -fmax-errors=3

xdisplay: xdisplay.h xdisplay.cc
	g++ -shared -Wl,--export-dynamic xdisplay.cc -fPIC -lX11 -lGL -lboost_python38 -lpython3.8 -I /usr/include/python3.8 -I ${VIRTUAL_ENV}/include -o xdisplay.so -std=c++2a -fmax-errors=3

