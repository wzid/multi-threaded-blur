default: build

build:
	@echo Building...
	g++ -o blur -O2 -pthread -std=c++14 main.cpp
	@echo Finished!