#include <stdio.h>

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

int main() {
    for (int i = 1; i <= 10; i++) {
        printf("fact(%d)=%d  fib(%d)=%d\n", i, factorial(i), i, fibonacci(i));
    }
    return 0;
}
