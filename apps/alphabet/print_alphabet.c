#include "alphabet.h"

void print_alphabet()
{
  long i;

  for (i = 0; i < N_ALPHABET_SIZE; i = i + 1) {
    *(long*)0x100000208 = array[i]; // print the charactor to STDOUT
  }

  *(long*)0x100000208 = 10; // new line

  commitd; // commit and delete watcher type
}
