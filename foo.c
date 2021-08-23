#include <stdio.h>

int foo() { printf("foo\n"); return 2; }
int bar(int x, int y) { printf("%d\n", x + y); return x + y; }
int goo(int x, int y, int z, int s, int t, int u) {
	printf("%d\n", x + y + z + s + t + u);
	return (x + y + z + s + t + u);
}
