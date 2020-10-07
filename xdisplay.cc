#include "xdisplay.h"

#include "pyutils.h"
#include "xconfig.h"

BOOST_PYTHON_MODULE(xdisplay) {
    using namespace boost;
    using namespace py;

    class_<XDisplay, noncopyable>(
        "XDisplay", init<>())
        //.add_property("run_loop", &XDisplay::run_loop)
    ;
    //py::class_<XWindow, boost::noncopyable>(
        //"XWindow", py::init<XDisplay &display>())
        //.add_property("area", &_GLRenderer::get_area)
    //;
    class_<XConfig>("XConfig", init<dict>());
    def("test", test);
}


