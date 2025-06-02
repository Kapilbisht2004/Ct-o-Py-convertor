#include<stdio.h>
#include<stdbool.h>
/*this is my code
to be converted*/

#define rr 10
int calculateSum(int a, int b)
{
    int result;
    result = a + b;
    return result;
}

int main() {
    int arr[2];
    arr[0]=4;
    arr[1]=6;
    int a = 10;
    float b = 3.14;
    char c = 'x';
    bool flag = true;

    printf("Value of a: %d\n", a);
    scanf("%d", &a);

    if (a > 5) {
        print("Greater than 5");
    } else {
        print("Less than or equal to 5");
    }

    for (int i = 0; i < a; i = i + 1) {
        print("Loop iteration");
    }

    for (int i = 0; i < a; i = i *5) {
        print("Loop iteration");
    }

    while (a > 0) {
        a = a - 1;
        print("Counting down");
    }
    int x = calculateSum(a,b);
    return 0;
}
