#include <stdio.h>
#include <stdbool.h>

// Simple macro
#define PI 3.14

// Macro function
#define SQUARE(x) ((x) * (x))

// Function to calculate sum of two integers
int calculateSum(int a, int b, int arr[]) {
    for (int i = 0; i < 2; i = i + 1) {
        printf("%d Element of array: %d\n", i, arr[i]);
    }
    int result = a + b;
    return result;
}

int main() {
    int arr[2];
    arr[0] = 4;
    arr[1] = 6;

    int a = 10;
    int b = 3;
    char c = 'x';
    bool flag = true;

    printf("Value of a: %d\n", a);
    printf("PI value from macro: %.2f\n", PI);
    printf("Square of b using macro function: %d\n", SQUARE(b));

    printf("Enter a new value for a: ");
    scanf("%d", &a);

    // Simple if-else
    if (a > 5) {
        printf("Greater than 5\n");
    } else {
        printf("Less than or equal to 5\n");
    }

    // 🟢 if-else if-else block for value of b
    if (b == 0) {
        printf("b is zero\n");
    } else if (b == 1) {
        printf("b is one\n");
    } else if (b == 2) {
        printf("b is two\n");
    } else {
        printf("b is greater than two\n");
    }

    // Normal loop
    for (int i = 0; i < a; i = i + 1) {
        printf("Loop iteration %d\n", i);
    }

    // Loop with multiplicative increment
    for (int i = 1; i < a; i = i * 6) {
        printf("Multiplicative loop iteration: %d\n", i);
    }

    // While loop
    while (a > 0) {
        a = a - 1;
        printf("Counting down: %d\n", a);
    }

    int x = calculateSum(a, b, arr);
    printf("Sum from function: %d\n", x);

    return 0;
}
