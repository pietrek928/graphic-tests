run: models.e
	./models.e

models.e: models.cpp
	g++ models.cpp -o models.e -O2 -lGL -lGLEW -lglfw

clean:
	rm -f models.e
