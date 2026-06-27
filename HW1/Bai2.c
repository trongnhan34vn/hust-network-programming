#include <stdio.h>
#include <math.h>

typedef struct point {
    double x;
    double y;
} point_t;

typedef struct circle {
    point_t center;
    double radius;
} circle_t;

int is_in_circle(point_t *p, circle_t *c) {
    // khoảng cách đến tâm 
    double dx = p->x - c->center.x;
    double dy = p->y - c->center.y;

    // kiểm tra khoảng cách đến tâm có nhỏ hơn bán kính không
    return sqrt(dx * dx + dy * dy) < c->radius;
}

int main() {
    circle_t c;
    scanf("%lf %lf %lf", &c.center.x, &c.center.y, &c.radius);

    point_t p;
    scanf("%lf %lf", &p.x, &p.y);

    if (is_in_circle(&p, &c))
        printf("YES\n");
    else
        printf("NO\n");

    return 0;
}
