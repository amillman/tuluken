#include "delay.h"

const int SAMPLE_SIZE = 10;
const int WINDOW = 6;

static int a[SAMPLE_SIZE] = { 1, 2, 3, 4, 2, 0, -2, 0, 1, 2 };
static int b[SAMPLE_SIZE] = { 0, -2, -1, 0, 1, 2, 3, 4, 2, 0, };

int main(int argc, char **argv)
{
    int result = offset(a, b, SAMPLE_SIZE, WINDOW);
    printf("%d\n", result);
    return 1;
}

// result > 0: b is delayed signal of a
// result < 0: a is delayed signal of b
int offset(int* a, int* b, int n, int w)
{
    float highestCorrelation = 0;
    int highestCorrelationOffest = 0;
    for (int i = -w; i <= w; i++) {
        float correlation = 0;
        for (int j = 0; j < n; j++) {
            int bIndex = i + j;
            if (bIndex < 0 || bIndex >= n) {
                continue;
            }
            int aVal = a[j];
            int bVal = b[bIndex];
            correlation += aVal * bVal;
        }
        correlation /= n - w;
        //printf("%f\n", correlation);
        if (correlation > highestCorrelation) {
            highestCorrelation = correlation;
            highestCorrelationOffest = i;
        }
    }

    return highestCorrelationOffest;
}
