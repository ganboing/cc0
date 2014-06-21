.PHONY: all common_lib kmeans-from-pmem.o clean

export CFLAGS=-DFROM_PMEM

all: common_lib kmeans-from-pmem.o
	linker -g -o kmeans-from-pmem.bin kmeans-from-pmem.o ../kmeans_common/init.o ../kmeans_common/on_finish.o ../kmeans_common/output.o ../kmeans_common/rand.o ../kmeans_common/run.o ../kmeans_common/libi0/libi0rt.o

common_lib:
	cd ../kmeans_common && $(MAKE)

kmeans-from-pmem.o:
	il_gen $(CFLAGS) $(CC0DEBUG) -o kmeans-from-pmem.o kmeans-from-pmem.c

clean:
	rm -rf kmeans-from-pmem.o kmeans-from-pmem.bin
	cd ../kmeans_common && $(MAKE) clean
