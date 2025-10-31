#CXX = clang++
CFLAGS=-Ofast -flto -ffloat-store -fexcess-precision=fast -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules -ffp-contract=fast -march=native -mtune=native -fomit-frame-pointer -Wall -Wextra#-march=native -mtune=native -g -Og
DEBUG_FLAGS = -march=native -mtune=native -g -Og -Wall -Wextra
ODIR = obj
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cc)
DEPS = $(wildcard $(SRCDIR)/*.h)
OBJ = $(patsubst $(SRCDIR)/%.cc,$(ODIR)/%.o,$(SOURCES))

engine: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) 

$(ODIR)/%.o: $(SRCDIR)/%.cc $(DEPS)
	@mkdir -p $(ODIR)
	$(CXX) -c $< -o $@ $(CFLAGS) 

.PHONY: debug clean

debug: $(OBJ)
	@mkdir -p $(ODIR)
	$(CXX) -o $@ $^ $(DEBUG_FLAGS)

clean:
	rm -f $(ODIR)/*.o