#CXX = clang++
CFLAGS=-Ofast -flto -ffloat-store -fexcess-precision=fast -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules -ffp-contract=fast -march=native -mtune=native -fomit-frame-pointer -Wall -Wextra#-march=native -mtune=native -g -Og
ODIR = obj
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cc)
DEPS = src/board.h src/constants.h
OBJ = $(patsubst $(SRCDIR)/%.cc,$(ODIR)/%.o,$(SOURCES))

engine: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) 

$(ODIR)/%.o: $(SRCDIR)/%.cc $(DEPS)
	@mkdir -p $(ODIR)
	$(CXX) -c $< -o $@ $(CFLAGS) 

clean:
	rm -f $(ODIR)/*.o