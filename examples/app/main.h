#ifndef __my_app_h__
#define __my_app_h__

/* Include the platform header for the target platform. */
#include <flipper/atsam4s/atsam4s.h>

/* Put main in its own section. */
void main(void) __attribute__((section (".main")));

#endif