.PHONY: all clean

all: init on_finish output rand run

clean:
	rm -rf init.o on_finish.o output.o rand.o run.o

init:
	$(CC0_CC) $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o init.o ../kmeans_common/init.c

on_finish:
	$(CC0_CC) $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o on_finish.o ../kmeans_common/on_finish.c

output:
	$(CC0_CC) $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o output.o ../kmeans_common/output.c

rand:
	$(CC0_CC) $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o rand.o ../kmeans_common/rand.c

run:
	$(CC0_CC) $(KMEANS_EXP) $(CFLAGS) $(CC0DEBUG) -o run.o ../kmeans_common/run.c
