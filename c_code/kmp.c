#include <iostream>
#include <cstring>
using namespace std;

int main() {
    char TIPS[] = "TEXT: %s%cPATTERN: %s%cANSWER: ";
  	char number[] = "%d ";
  	char charactor[] = "%c";
  	char NO_ANSWER[] = "none";
  	char text[] = "anpanman";
  	char pattern[] = "an";
  	int answer[100];
  	int a = 0;
  	int m = strlen(text);
  	int n = strlen(pattern);

    int i = 0;
    int j = -1;
    int next[100];
    int x;
    int y;
    int z;
    next[0] = -1;
    while (i < n) {
        while (j > -1 && pattern[i] != pattern[j]) {
            j = next[j];
        }
        i += 1;
        j += 1;
        if ((i != n) && (j != m) && (pattern[i] == pattern[j]))
            next[i] = next[j];
        else if ((i == n) && (j == m))
            next[i] = next[j];
        else
            next[i] = j;
    }

    i = 0;
    j = 0;
    while (j < m) {
        while (i > -1 && text[j] != pattern[i])
            i = next[i];
        i += 1;
        j += 1;
        if (i >= n) {
            answer[a] = j - i;
            a += 1;
            i = next[i];
        }
   }

   // printf(TIPS, text, 10, pattern, 10);
   // if (a == 0)
       // printf(NO_ANSWER);
   // else {
       for (i = 0; i < a; i += 1) {
           cout << answer[i] << endl;
       }
   // }
   // printf(charactor, 10);
   return 0;
}
