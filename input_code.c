#include<stdio.h>
/*this is my code
to be converted*/
int calculateSum(int a, int b)
{
    int result;
    result = a + b;
    return result;
}

void main()
{
    int x = 10;
    int y;
    y = 20;
    if (x > 5)
    {
        x = x + (y * 2);
        y=calculateSum(x, y); // Expression statement with function call
    }
    else
    {
        x = 0;
    }
    while (x < 15)
    {
        x = x + 1;
    }
    for (int i = 0; i < 3; i = i * 2)
    {
        y = y - 1;
    }
    return 0; // Return with no value (void context)
}