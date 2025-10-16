#CXX = clang++
CFLAGS=-Ofast -march=native -mtune=native -fomit-frame-pointer -Wall -Wextra#-march=native -mtune=native -g -Og
DEPS = board.h constants.h
ODIR = obj
_OBJ = constants.o board.o engine.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

engine: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) 

$(ODIR)/%.o: %.cc $(DEPS)
	$(CXX) -c $< -o $@ $(CFLAGS) 

clean:
	rm -f $(ODIR)/*.o