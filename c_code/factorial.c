#include <stdio.h>
#include <string.h>

int factorial(int n) {
    int rst;
    char db[] = "%d%c";
    printf(db, n, 10);
    if (n == 0)
        rst = 1;
    else
        rst = n;
    if (n != 1 && n != 0)
        rst = n * factorial(n-1);
    return rst;
}

int main() {
    char result[] = "INPUT: %d%cRESULT: %d";
    int input = 4;
    int a = factorial(input);
    printf(result, input, 10, a);
    return 0;
}
