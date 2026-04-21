#include <stdio.h>
#include <math.h>

int isPrime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i <= (int)sqrt((double)n); i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main() {
    printf("Primes up to 50: ");
    for (int i = 2; i <= 50; i++) {
        if (isPrime(i)) printf("%d ", i);
    }
    printf("\n");
    printf("gcd(48, 18) = %d\n", gcd(48, 18));
    return 0;
}
