.PHONY: all common_lib kmeans-stdin-to-pmem.o clean

export CFLAGS=-DFROM_PMEM

all: common_lib kmeans-stdin-to-pmem.o
	linker $(CC0DEBUG) -o kmeans-stdin-to-pmem.bin kmeans-stdin-to-pmem.o ../kmeans_common/init.o ../kmeans_common/on_finish.o ../kmeans_common/output.o ../kmeans_common/rand.o ../kmeans_common/run.o ../kmeans_common/libi0/libi0rt.o

common_lib:
	cd ../kmeans_common && $(MAKE)

kmeans-stdin-to-pmem.o:
	il_gen $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o kmeans-stdin-to-pmem.o kmeans-stdin-to-pmem.c

clean:
	rm -rf kmeans-stdin-to-pmem.o kmeans-stdin-to-pmem.bin
	cd ../kmeans_common && $(MAKE) clean
