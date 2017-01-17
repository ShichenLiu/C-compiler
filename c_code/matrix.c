#include <stdio.h>
#include <string.h>

int main() {
    int a[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int b[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int len = 3;
    int c[9];
    char number[] = "%d ";
    char charactor[] = "%c";

    int i;
    int j;
    int m;
    for (i = 0; i < len; i += 1) {
        for (j = 0; j < len; j += 1) {
            c[i*len+j] = 0;
            for (m = 0; m < len; m += 1)
                c[i*len+j] += a[i*len+m] * b[m*len+j];
            printf(number, c[i*len+j]);
        }
        printf(charactor, 10);
    }
    return 0;
}
