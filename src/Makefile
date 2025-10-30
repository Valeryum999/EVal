#CXX = clang++
CFLAGS=-Ofast -flto -ffloat-store -fexcess-precision=fast -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules -ffp-contract=fast -march=native -mtune=native -fomit-frame-pointer -Wall -Wextra#-march=native -mtune=native -g -Og
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