#include <stdint.h>
#include <stdio.h>

int fib(int n){
        if(n == 1 || n == 0) return n;
        else return fib(n-1) + fib(n-2);
}

int main(){
        printf("Hallo, ich bin ein programmes\n");
        int i;
        for(i = 1; i<100; i++)
                printf("[%d] fib(%d) = %d\n",getpid(), i, fib(i));
        return 0;
}
