#include <stdio.h>

int multiply(int a) {
    return a * a;
}

int calculate(int limit) {
    int sum = 0;
    for (int j = 1; j <= limit; j++) {
        sum = sum + multiply(j);
    }
    return sum;
}

int main() {
    int val = calculate(20);
    if (val > 200) {
        printf("Big: %d\n", val);
    } else {
        printf("Tiny: %d\n", val);
    }
    return 0;
}
