#include "tpl_inst.h"

int main() {
    int x {};
    double y {};

    tpl_func(x);
    tpl_func(y);

    test t;
    t.tpl_method(x);
    // t.tpl_method(y);
    return 0;
}