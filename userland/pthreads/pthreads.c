#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "debug-shmac.h"

#define DEBUG_SYS_INT_STATUS ((volatile unsigned long*)(0xffff0020))
#define DEBUG_SYS_OUT_DATA ((volatile unsigned long*)(0xffff0000))

void *print_message_function1( void *ptr );
void *print_message_function2( void *ptr );

int main(void)
{
     pthread_t thread1, thread2;
     const char *message1 = "Thread 1";
     const char *message2 = "Thread 2";

    /* Create independent threads each of which will execute function */

     pthread_create( &thread1, NULL, print_message_function1, (void*) message1);
     pthread_create( &thread2, NULL, print_message_function2, (void*) message2);

     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL);

     printf("Thread 1 returns");
     printf("Thread 2 returns");
     exit(0);
}

void *print_message_function1( void *ptr )
{
     printf("Thread 1 running");
}

void *print_message_function2( void *ptr )
{
     printf("Thread 2 running");
}

