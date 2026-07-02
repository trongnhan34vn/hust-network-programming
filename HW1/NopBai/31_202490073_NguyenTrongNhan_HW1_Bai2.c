/* HW1 - Bai 2: Kiểm tra điểm nằm trong hình tròn
 * Input:  dòng 1 - x y r (tọa độ tâm và bán kính)
 *         dòng 2 - x y   (tọa độ điểm cần kiểm tra)
 * Output: YES nếu điểm nằm bên trong (không tính biên), NO nếu ngược lại
 */

#include <stdio.h>
#include <math.h>

/* Cấu trúc biểu diễn một điểm 2D */
typedef struct point {
    double x;
    double y;
} point_t;

/* Cấu trúc biểu diễn hình tròn với tâm và bán kính */
typedef struct circle {
    point_t center;
    double radius;
} circle_t;

/* Kiểm tra điểm p có nằm bên trong hình tròn c không.
 * Dùng so sánh chặt (<): điểm nằm trên đường tròn → trả về 0 (NO). */
int is_in_circle(point_t *p, circle_t *c) {
    double dx = p->x - c->center.x;
    double dy = p->y - c->center.y;
    /* Khoảng cách Euclid từ p đến tâm c */
    return sqrt(dx * dx + dy * dy) < c->radius;
}

int main() {
    /* Dòng 1: tọa độ tâm (x, y) và bán kính r */
    circle_t c;
    scanf("%lf %lf %lf", &c.center.x, &c.center.y, &c.radius);

    /* Dòng 2: tọa độ điểm cần kiểm tra */
    point_t p;
    scanf("%lf %lf", &p.x, &p.y);

    if (is_in_circle(&p, &c))
        printf("YES\n");
    else
        printf("NO\n");

    return 0;
}
