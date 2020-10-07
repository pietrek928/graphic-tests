#ifndef __GTK_UTILS_H_
#define __GTK_UTILS_H_


#include <pygobject-3.0/pygobject.h>
#include <gtk/gtk.h>

template<auto fp, class To, class Tr, class... Targs>
Tr __gtk_cbk(Targs... args, To *obj) {
    return (obj->*fp)(args...);
}

template<auto fp, class To, class Tr, class... Targs>
inline auto __gtk_cbk_wrap(Tr(To::*_fp)(Targs...)) {
    return (Tr(*)(Targs..., To *)) __gtk_cbk<fp, To, Tr, Targs...>;
}

template<auto fp>
inline auto gtk_cbk() {
    return __gtk_cbk_wrap<fp>(fp);
}

#define GCBK(f) G_CALLBACK(gtk_cbk<f>())


#endif /* __GTK_UTILS_H_ */

