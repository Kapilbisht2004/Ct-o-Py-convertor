def calculateSum(a, b):
        result = (a + b)
        return result
def main():
        x = 10
        y = 20
        if (x > 5):
            x = (x + (y * 2))
            y = calculateSum(x, y)
        else:
            x = 0
        while (x < 15):
            x = (x + 1)
        i = 0
        while (i < 3):
            y = (y - 1)
            i = (i * 2)
        return 0
