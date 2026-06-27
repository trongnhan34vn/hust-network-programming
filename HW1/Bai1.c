#include <stdio.h>
#include <stdlib.h>

char *subStr(char *s1, int offset, int number)
{
    char *result = (char *)malloc((number + 1) * sizeof(char));

    for (int i = 0; i < number; i++)
    {
        // do offset đếm từ 1, còn index string đếm từ 0
        result[i] = s1[offset - 1 + i];
    }
    result[number] = '\0';

    if (result == NULL)
    {
        return NULL;
    }

    return result;
}

int main()
{
    printf("Enter offset: ");
    int offset;
    scanf("%d", &offset);

    printf("Enter number: ");
    int number;
    scanf("%d", &number);

    char s1[100];
    printf("Enter a string: ");
    scanf("%s", s1);

    char *result = subStr(s1, offset, number);
    if (result != NULL)
    {
        printf("Substring: -%s-\n", result);
        free(result);
    }
    else
    {
        printf("Error substring.\n");
    }

    return 0;
}
