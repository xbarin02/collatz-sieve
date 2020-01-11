CFLAGS+=-std=c89 -pedantic -Wall -Wextra -march=native -O3 -D_BSD_SOURCE -Wno-unused-result -Wno-uninitialized
LDFLAGS=
LDLIBS+=
BINS=hist fdc fdd

CFLAGS+=$(EXTRA_CFLAGS)
LDFLAGS+=$(EXTRA_LDFLAGS)
LDLIBS+=$(EXTRA_LDLIBS)

.PHONY: all
all: $(BINS)

.PHONY: clean
clean:
	$(RM) -- $(BINS)

.PHONY: distclean
distclean: clean
	$(RM) -- *.gcda
