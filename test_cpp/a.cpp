#include <stdio.h>

int square(int x) {
    return x * x;
}

int sumUpTo(int n) {
    int total = 0;
    for (int i = 1; i <= n; i++) {
        total = total + square(i);
    }
    return total;
}

int main() {
    int result = sumUpTo(10);
    if (result > 100) {
        printf("Large: %d\n", result);
    } else {
        printf("Small: %d\n", result);
    }
    return 0;
}
