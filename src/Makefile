CFLAGS+=-std=c89 -pedantic -Wall -Wextra -march=native -O3 -D_XOPEN_SOURCE
LDLIBS+=

ifndef $(SIEVE_LOGSIZE)
	SIEVE_LOGSIZE=16
endif
CFLAGS+=-DSIEVE_LOGSIZE=$(SIEVE_LOGSIZE)

ifeq ($(USE_LIBGMP), 1)
	CFLAGS+=-D_USE_GMP
	LDLIBS+=-lgmp
endif

ifeq ($(USE_OPENMP), 1)
	CFLAGS+=-fopenmp
	LDLIBS+=-fopenmp
endif

CFLAGS+=$(EXTRA_CFLAGS)
LDLIBS+=$(EXTRA_LDLIBS)

LDFLAGS=
BINS=max sieve stats fsieve

.PHONY: all
all: $(BINS)

.PHONY: clean
clean:
	$(RM) -- $(BINS)

.PHONY: distclean
distclean: clean
	$(RM) -- *.gcda
