/* HW1 - Bai 1: Cắt chuỗi con (substring)
 * Input:  dòng 1 - hai số nguyên offset number
 *         dòng 2 - chuỗi nguồn (tối đa 80 ký tự)
 * Output: chuỗi con bao bọc bởi dấu '-', ví dụ -edu-
 * offset tính từ 0 (chỉ số mảng ký tự)
 */

#include <stdio.h>
#include <stdlib.h>

/* Cắt chuỗi con từ s1 bắt đầu tại vị trí offset (đếm từ 0), lấy number ký tự.
 * Nếu number vượt quá phần còn lại từ offset, trả về phần còn lại.
 * Trả về con trỏ heap; caller phải free().
 * Trả về NULL nếu offset không hợp lệ (< 0 hoặc >= strlen) hoặc malloc thất bại. */
char *subStr(char *s1, int offset, int number) {
    /* Tính độ dài s1 không dùng string.h */
    int len = 0;
    while (s1[len] != '\0') len++;

    /* Kiểm tra offset hợp lệ (0-indexed: phải nằm trong [0, len-1]) */
    if (offset < 0 || offset >= len) return NULL;

    /* Tính số ký tự thực sự có thể lấy từ vị trí offset */
    int remaining = len - offset;
    if (number > remaining) number = remaining;  /* Lấy phần còn lại nếu number quá lớn */

    /* Cấp phát động cho kết quả (không dùng mảng tĩnh) */
    char *result = (char *)malloc((number + 1) * sizeof(char));
    if (result == NULL) return NULL;

    /* Sao chép number ký tự bắt đầu từ index offset */
    for (int i = 0; i < number; i++)
        result[i] = s1[offset + i];
    result[number] = '\0';

    return result;
}

int main() {
    int offset, number;
    /* Dòng 1: offset và number */
    scanf("%d %d", &offset, &number);

    /* Dòng 2: chuỗi nguồn (tối đa 80 ký tự, không dùng mảng tĩnh cho kết quả) */
    char s1[81];
    scanf("%s", s1);

    char *result = subStr(s1, offset, number);
    if (result != NULL) {
        printf("-%s-\n", result);
        free(result);
    } else {
        printf("Error: invalid offset\n");
    }

    return 0;
}
