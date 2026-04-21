#include <stdio.h>

void selectionSort(int a[], int size) {
    for (int i = 0; i < size - 1; i++) {
        int minIdx = i;
        for (int j = i + 1; j < size; j++) {
            if (a[j] < a[minIdx]) {
                minIdx = j;
            }
        }
        int tmp = a[i];
        a[i] = a[minIdx];
        a[minIdx] = tmp;
    }
}

void showArray(int a[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", a[i]);
    }
    printf("\n");
}

int main() {
    int nums[] = {64, 34, 25, 12, 22, 11, 90};
    int n = 7;
    printf("Before: ");
    showArray(nums, n);
    selectionSort(nums, n);
    printf("After:  ");
    showArray(nums, n);
    return 0;
}
