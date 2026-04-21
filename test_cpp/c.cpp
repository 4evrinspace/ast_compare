#include <stdio.h>

int cube(int x) {
    return x * x * x;
}

int sumCubes(int n) {
    int total = 0;
    for (int i = 1; i <= n; i++) {
        total = total + cube(i);
    }
    return total;
}

int main() {
    int result = sumCubes(10);
    if (result > 1000) {
        printf("Large: %d\n", result);
    } else {
        printf("Small: %d\n", result);
    }
    return 0;
}
