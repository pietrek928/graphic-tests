#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "pyutils.h"

class XConfig {
    py::dict cfg;

    public:

    XConfig(py::dict cfg) : cfg(cfg) {
    }

    template<class T>
    T get(const char *name) const {
        return py::extract<T>(
            cfg.get(name)
        );
    }

    template<class T>
    T get(const char *name, T default_) const {
        if cfg.has_key(name) {
            return py::extract<T>(
                cfg.get(name)
            );
        } else {
            return default_;
        }
    }

    XConfig child(const char *name) {
        return XConfig(
            py::dict(
                cfg.get(name)
            )
        );
    }
};

#endif /* __CONFIG_H_ */

