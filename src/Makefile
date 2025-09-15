CFLAGS=-Ofast -march=native -Wall -Wextra #-Ofast -march=native -g -Og
DEPS = board.h
ODIR = obj
_OBJ = constants.o board.o engine.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

engine: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) 

$(ODIR)/%.o: %.cc $(DEPS)
	$(CXX) -c $< -o $@ $(CFLAGS) 

clean:
	rm -f $(ODIR)/*.o