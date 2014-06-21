.PHONY: all clean 

all:
	@ cc0 -o kmeans-stdin.bin kmeans-stdin.c --debug >cc0.log && \
	cc0 -o kmeans-from-pmem.bin kmeans-from-pmem.c --debug >cc0.log && \
	cc0 -o kmeans-stdin-to-pmem.bin kmeans-stdin-to-pmem.c --debug >cc0.log && \
	echo -e "\nYou have successfully built kmeans\n"

kmeans-stdin:
	

clean:
	rm -f kmeans-stdin.bin kmeans-from-pmem.bin kmeans-stdin-to-pmem.bin
