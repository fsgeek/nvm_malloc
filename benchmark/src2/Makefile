CXX ?= g++
CXXFLAGS := -std=c++11 -Wall -Isrc -I../../src -I../../makalu_alloc/include
debug:   CXXFLAGS += -O0 -ggdb
release: CXXFLAGS += -O3
LDFLAGS := -L../.. -ltbb -lpthread -L../../makalu_alloc/lib -lmakalu -lpmemcto -lpmemobj -lpmem -lnvmmalloc

SRCDIR := src
BUILDDIR := build
BINARIES := bench_fastalloc bench_linkedlist bench_recovery bench_alloc_free bench_alloc_free_alloc
TARGETS := $(addprefix $(BUILDDIR)/, $(BINARIES))

all: release

release: $(TARGETS)

debug: $(TARGETS)

$(BUILDDIR)/bench_recovery: $(SRCDIR)/bench_recovery.cpp $(SRCDIR)/common.h $(SRCDIR)/common.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -o $@ $< $(SRCDIR)/common.cpp $(LDFLAGS) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_PMDK -o $@_pmdk $< $(SRCDIR)/common.cpp $(LDFLAGS) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_PMOBJ -o $@_pmobj $< $(SRCDIR)/common.cpp $(LDFLAGS) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_MAKALU -o $@_makalu $< $(SRCDIR)/common.cpp $(LDFLAGS) $(LDFLAGS)

$(BUILDDIR)/%: $(SRCDIR)/%.cpp $(SRCDIR)/common.h $(SRCDIR)/common.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DUSE_MALLOC -o $@ $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -o $@_nvm $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -DHAS_CLFLUSHOPT -o $@_nvm_clflushopt $(LDFLAGS) $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -DHAS_CLWB -o $@_nvm_clwb $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -o $@_nvm_noflush $(LDFLAGS)noflush $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -o $@_nvm_nofence $(LDFLAGS)nofence $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_NVM_MALLOC -o $@_nvm_none $(LDFLAGS)none $< $(SRCDIR)/common.cpp $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -DUSE_PMDK -o $@_pmdk $< $(SRCDIR)/common.cpp $(LDFLAGS) -lpmemcto -lpmem
	$(CXX) $(CXXFLAGS) -DUSE_PMOBJ -o $@_pmobj $< $(SRCDIR)/common.cpp $(LDFLAGS) -lpmemcto -lpmem
	$(CXX) $(CXXFLAGS) -DUSE_MAKALU -o $@_makalu $< $(SRCDIR)/common.cpp $(LDFLAGS) -lpmemcto -lpmem

clean:
	@rm -rf $(BUILDDIR)

.PHONY: clean
