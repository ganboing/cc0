#include "alphabet.h"

void create_alphabet()
{
  long i;

  for (i = 0; i < N_ALPHABET_SIZE; i = i + 1) {
    array[i] = 65 + i; // ASCII code for charactors. e.g. 'A' = 65 = 65+0
  }

  commit; // runner commits
}
