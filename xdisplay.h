#ifndef __X_DISPLAY_H_
#define __X_DISPLAY_H_

#include <stdint.h>
#include <unistd.h>
#include "utils.h"

#include <X11/Xlib.h>

#include "gl_renderer.cc"
#include "fps.h"

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

class XDisplay {
    public:

    Display* display = NULL;
    Screen* screen = NULL;
    int screenId = -1;
    GLint majorGLX = 0, minorGLX = 0;

    RARE_FUNC
    void handle_opening_display_error() {
        throw std::runtime_error("Opening X display failed");
    }

    void clear() {
        if (likely(screen)) {
            //XFree(screen);
            screen = NULL;
        }
        if (likely(display)) {
            XCloseDisplay(display);
            display = NULL;
        }
    }

    public:

    XDisplay(EmptyObject _) {}

    RARE_FUNC
    XDisplay() {
        display = XOpenDisplay(NULL);
        if (unlikely(display == NULL)) {
            handle_opening_display_error();
        }
        screen = DefaultScreenOfDisplay(display);
        screenId = DefaultScreen(display);

	    glXQueryVersion(display, &majorGLX, &minorGLX);
        DBG_STREAM(X) << "GLX server version: " << majorGLX << "." << minorGLX << DBG_ENDL;
    }

    ~XDisplay() {
        clear();
    }

    operator Display*() const {
        return display;
    }

    operator Screen*() const {
        return screen;
    }

    operator int() const {
        return screenId;
    }
};

class GLContext {
    XDisplay display;
    GLXContext context = NULL;
    GLXFBConfig visualFBC;

    RARE_FUNC
    void handle_creation_error() {
        throw std::runtime_error("OpenGL context creation failed");
    }

    RARE_FUNC
    void handle_fbconfig_error() {
        throw std::runtime_error("Failed to retrieve framebuffer");
    }

    RARE_FUNC
    void handle_visual_error() {
        throw std::runtime_error("Could not create correct visual window");
    }

    RARE_FUNC
    void handle_wrong_screen_id(int wrong_id) {
        throw std::runtime_error("Visual config: rong screenId: " + std::to_string(wrong_id));
    }

    public:

    auto get_visual() const { // free !!!!!!!!!
        return glXGetVisualFromFBConfig(display, visualFBC);
    }

    GLContext() {
        // !!!!!!!!!!!!!!!!!!!!!!!!

        GLint glxAttribs[] = {
            GLX_X_RENDERABLE    , True,
            GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
            GLX_RENDER_TYPE     , GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
            GLX_RED_SIZE        , 8,
            GLX_GREEN_SIZE      , 8,
            GLX_BLUE_SIZE       , 8,
            GLX_ALPHA_SIZE      , 8,
            GLX_DEPTH_SIZE      , 24,
            GLX_STENCIL_SIZE    , 8,
            GLX_DOUBLEBUFFER    , True,
            //GLX_SAMPLE_BUFFERS  , 1,            // <-- MSAA
            //GLX_SAMPLES         , 4,            // <-- MSAA
            None
            /*GLX_X_RENDERABLE    , True,
            GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
            GLX_RENDER_TYPE     , GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
            GLX_RED_SIZE        , 8,
            GLX_GREEN_SIZE      , 8,
            GLX_BLUE_SIZE       , 8,
            GLX_ALPHA_SIZE      , 8,
            GLX_DEPTH_SIZE      , 24,
            GLX_STENCIL_SIZE    , 8,
            GLX_DOUBLEBUFFER    , True,
            None*/
        };

        int fbcount;
        GLXFBConfig *fbc = glXChooseFBConfig(display, display, glxAttribs, &fbcount);
        if (unlikely(fbc == NULL)) {
            handle_fbconfig_error();
        }

        int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
        for (int i = 0; i < fbcount; i++) {
            XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
            if (vi != NULL) {
                int samp_buf, samples;
			    glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
			    glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES       , &samples );
                DBG_STREAM(GL) << "Max buffers: " << samp_buf << ENDL;
                DBG_STREAM(GL) << "Samples: " << samples << ENDL;

                if ( best_fbc < 0 || (samp_buf && samples > best_num_samp) ) {
                    best_fbc = i;
                    best_num_samp = samples;
                }
                if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
                    worst_fbc = i;
                worst_num_samp = samples;

                XFree( vi );
            }
        }
        visualFBC = fbc[best_fbc];
        XFree(fbc);

        auto visual = get_visual();
        if (unlikely(visual == NULL)) {
            handle_visual_error();
        }
        if (unlikely(display != visual->screen)) {
            handle_wrong_screen_id(visual->screen);
        }

        // !!!!!!!!!!!!!!!!!!!!!!
        
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
	    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

        int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 2,
            GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };
        
        //const char *glxExts = glXQueryExtensionsString(display, display); // TODO: set ?
        //std::cout << glxExts << ENDL;
        //if (!isExtensionSupported(glxExts, "GLX_ARB_create_context")) {
            //DBG_STREAM(GL) << "GLX_ARB_create_context not supported" << ENDL;
            //context = glXCreateNewContext( display, bestFbc, GLX_RGBA_TYPE, 0, True );
        //}
        //else {
            context = glXCreateContextAttribsARB( display, visualFBC, 0, true, context_attribs );
        //}
        if (unlikely(context == NULL)) {
            handle_creation_error();
        }
        XSync(display, False);
        if (!glXIsDirect (display, context)) {
            DBG_STREAM(GL) << "Indirect GLX rendering context obtained" << ENDL;
        } else {
            DBG_STREAM(GL) << "Direct GLX rendering context obtained" << ENDL;
        }
    }

    void clear() {
        if (likely(context)) {
            glXDestroyContext(display, context);
            context = NULL;
        }
    }

    ~GLContext() {
        clear();
    }

    operator Display*() const {
        return display;
    }

    operator XDisplay&() {
        return display;
    }

    operator GLXContext() const {
        return context;
    }
};


template<class v>
class BBox {
    v start, end;

    public:
    BBox() {}

    BBox(v start, v end) 
    : start(start), end(end) {}

    auto operator+(BBox b) {
        return BBox(
            start.min(b.start), end.max(b.end)
        );
    }

    auto pos() {
        return start;
    }

    auto size() {
        return end - start;
    }
};


class XWindow {
    Window window = 0;
    BBox<v2i> visible_box;

    public:
    XWindow() {
    }

    XWindow(GLContext &ctx) {
        XDisplay &display = ctx;
        visible_box = BBox(
            v2i(0, 0),
            v2i(320, 200)
        );

        auto visual = ctx.get_visual();

        auto root_window = DefaultRootWindow((Display*)display);
        XSetWindowAttributes windowAttribs;
        //windowAttribs.border_pixel = BlackPixel((Display*)display, display);
        //windowAttribs.background_pixmap = None;
        //windowAttribs.background_pixel = BlackPixel((Display*)display, display);
        //windowAttribs.override_redirect = True; ???
        windowAttribs.colormap = XCreateColormap(display, root_window, visual->visual, AllocNone);
        windowAttribs.event_mask = ExposureMask;
        auto size = visible_box.size();
        window = XCreateWindow(
            display, root_window, 0, 0, size.x, size.y,
            0, visual->depth, InputOutput, visual->visual,
            /*CWBackPixmap | CWBorderPixel | */CWColormap | CWEventMask, &windowAttribs
        );

        Atom atomWmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &atomWmDeleteWindow, 1);

        XSelectInput( // ???????
            display, window, // ExposureMask | TODO: handle visible bbox
            VisibilityChangeMask | FocusChangeMask
            | ButtonMotionMask | PointerMotionHintMask
            | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            | EnterWindowMask | LeaveWindowMask | StructureNotifyMask
            | KeyPressMask | KeyReleaseMask | GCGraphicsExposures
        );

        XClearWindow(display, window);
        XMapRaised(display, window);
    }

    XWindow(XWindow &&w) {
        window = w.window;
    }

    XWindow &operator=(XWindow &&w) {
        clear();
        window = w.window;
        w.window = 0;
        return *this;
    }

    operator Window() const {
        return window;
    }

    void bind(GLContext &ctx) {
        glXMakeCurrent(ctx, window, ctx);
    }

    //void resize(v2i &new_size, v2i &new_pos) {
        //size = new_size;
        //pos = new_pos;
    //}

    void start_rendering(GLContext &ctx) {
        bind(ctx);
        auto pos = visible_box.pos();
        auto size = visible_box.size();
        glViewport(pos.x, pos.y, size.x, size.y);
        glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void end_rendering(GLContext &ctx) {
        glXSwapBuffers(ctx, window);
    }

    void close(GLContext &ctx) {
        if (likely(window)) {
            //XFreeColormap(ctx, windowAttribs.colormap); !!!!!!!!!!
            XDestroyWindow(ctx, window);
            window = 0;
        }
    }

    void clear() {
        if (likely(window)) {
            window = 0;
        }
    }

    void set_box(v2i &pos, v2i &size) {
        visible_box = BBox(pos, pos+size);
    }

    void extend_box(v2i &pos, v2i &size) {
        visible_box = visible_box + BBox(pos, pos+size);
    }

    void reset_box() {
        visible_box = BBox(v2i(1000000, 1000000), v2i(0, 0));
    }
};

#include <unordered_map>
#include <unordered_set>

void test() {
    auto ctx = GLContext();
    
    auto w1 = XWindow(ctx);
    auto w2 = XWindow(ctx);

    std::unordered_map<Window, XWindow*> wm;
    wm.emplace((Window)w1, &w1);
    wm.emplace((Window)w2, &w2);

    w1.start_rendering(ctx);

    VertexArray<GL_TRIANGLES, v2, v2> quad_arr({
        -0.5f,  0.5f,    0.0f, 0.5f,
        -0.5f, -0.5f,    0.0f, 0.0f,
         0.5f, -0.5f,    0.5f, 0.0f,
        -0.5f,  0.5f,    0.0f, 0.5f,
         0.5f, -0.5f,    0.5f, 0.0f,
         0.5f, -0.5f,    0.5f, 0.5f,
    });
    quad_arr.attach_data_static();
    NormalBuffer buff(v2i(1024, 1024));

    FragmentShader fragment(
            "#version 330" ENDL
            "in vec2 vertTexCoord;"
            "out vec3 color;"
            "uniform sampler2D myTexture;"
            "uniform float col_ch;"
            "void main() {"
                "color = vec3("
                    "vertTexCoord.x*vertTexCoord.y,"
                    "1.0 - vertTexCoord.x*vertTexCoord.y,"
                    "col_ch"
                ");"
            "}"
    );

    VertexShader vertex(
            "#version 330" ENDL
            "layout(location = 0) in vec2 vertCoord;"
            "layout(location = 1) in vec2 texCoord;"
            "out vec2 vertTexCoord;"
            "void main() {"
                "gl_Position = vec4(vertCoord, 0, 1);"
                "vertTexCoord = texCoord;"
            "}"
    );

    Program drawing_program(vertex, fragment);
    auto loc = glGetUniformLocation(drawing_program, "col_ch"); // to class !!!   

    float a=0., b=-.01;
    FPSController fps(64.0f);

    std::unordered_set<Window> ww;
    XEvent ev;
    while (true) {
        if (XPending(ctx) > 0)
        do {
            XNextEvent(ctx, &ev);
            switch(ev.type) {
#if 0
                case Expose:{
                    auto &e = ev.xexpose;
                    v2i box_size(
                        e.width, e.height
                    );
                    v2i box_pos(
                        e.x, e.y
                    );
                    DBG_STREAM(X) << e.window << ": Expose " << box_size << ", Position: " << box_pos << ENDL;
                    auto w = wm[e.window];
                    if (ww.find(e.window) == ww.end()) {
                        //w->reset_box();
                        ww.insert(e.window);
                    }
                    w->extend_box(box_pos, box_size);
                    if (!e.count) {
                        ww.erase(e.window);
                    }
                    }break;
                case GraphicsExpose:{
                    if (!ev.xgraphicsexpose.count) {
                        auto &e = ev.xgraphicsexpose;
                        v2i new_size(
                            e.width, e.height
                        );
                        v2i new_pos(
                            e.x, e.y
                        );
                        DBG_STREAM(X) << "Graphics expose " << e.drawable << ": Resize " << new_size << ", Position: " << new_pos << ENDL;
                        //auto w = wm[e.window];
                        //w->resize(new_size, new_pos);
                    }
                    }break;
#endif
                case ConfigureNotify:{
                        auto &e = ev.xconfigure;
                        v2i box_size(
                            e.width, e.height
                        );
                        v2i box_pos(
                            //e.x, e.y
                            0, 0
                            //300, 300
                        );
                        DBG_STREAM(X) << "Configure " << e.window << ": Resize " << box_size << ", Position: " << box_pos << ENDL;
                        auto w = wm[e.window];
                        w->set_box(box_pos, box_size);
                    }break;
                case ClientMessage:
                    DBG_STREAM(X) << "Shutdown" << ENDL;
                    return;
                    break;
                case DestroyNotify:
                    DBG_STREAM(X) << "Destroy window" << ENDL;
                    break;
                default:
                    DBG_STREAM(X) << ev.xany.window << ": " << ev.type << DBG_ENDL;
                    break;
            }
        } while (XPending(ctx) > 0);
        buff.startDrawing();
        drawing_program.use();
        glSetUniform(loc, a);
        quad_arr.draw();
        buff.endDrawing();

        w1.start_rendering(ctx);
        buff.display();
        w1.end_rendering(ctx);
        w2.start_rendering(ctx);
        buff.display();
        w2.end_rendering(ctx);

        a += b;
        if (a < 0.0f  || a > 1.0f) {
            b = -b;
        }

        //fps.show_stats();
        fps.sleep();
    }
}

#endif /* __X_DISPLAY_H_ */

