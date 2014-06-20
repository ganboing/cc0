#include "alphabet.h"
 
void main()
{
  /* runner that creates the alphabet and stores them in array[] */
  runner create_alphabet ()
    using array[0,,N_ALPHABET_SIZE];
 
  /* watcher that prints out the alphabet in array[] */
  runner print_alphabet ()
    using array[0,,N_ALPHABET_SIZE]
    watching array[0,,N_ALPHABET_SIZE];
 
  commit;
}
 

