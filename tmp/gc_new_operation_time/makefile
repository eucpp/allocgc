SOURCE=test.cpp
CPPC=g++
LOAD_LIBRARY=-L /usr/local/lib/ -l allocgc -l pthread -l msmalloc

all: output

output: $(SOURCE)
	$(CPPC) -std=c++11 $(SOURCE) $(LOAD_LIBRARY) -o output

clean:
	rm -rf output
