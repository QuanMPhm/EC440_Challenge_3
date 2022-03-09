#include <stdio.h>

int main() {
    int a = 10;
    long int b = -20000;
    long int * c = &b;
    long int d = (long int) c;
    long int * e = (long int *) d;
    
    printf("%d %u %p %d %p %d\n", b, (long unsigned int) b, c, d, e, *e);
    
    return 0;
}