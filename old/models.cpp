#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>        /* include GLEW and new version of GL on Windows */
#include <GLFW/glfw3.h> /* GLFW helper library */

using namespace std;

template<class T>
class _v2 {
    public:
        typedef T Telem;

        T x,y;

        _v2(T x, T y) : x(x), y(y) {}
        _v2() : x(0), y(0) {}

        auto operator+(const _v2 v) {
            return _v2(x+v.x, y+v.y);
        }

        auto operator+=(const _v2 v) {
            x += v.x;
            y += v.y;
            return *this;
        }

        auto operator-(const _v2 v) {
            return _v2(x-v.x, y-v.y);
        }

        auto operator*(const _v2 v) {
            return _v2(x*v.x, y*v.y);
        }

        auto operator*(const T s) {
            return _v2(x*s, y*s);
        }

        auto dp(const _v2 v) {
            return x*v.x + y*v.y;
        }

        auto vp(const _v2 v) {
            return x*v.y - y*v.x;
        }

        auto rqlen() {
            return T(1)/(dp(*this));
        }
};
template<class Telem>
std::ostream &operator<<(std::ostream &os, const _v2<Telem> &v) {
    return os << "(" << v.x << ", " << v.y << ")";
}
template<class Telem>
Telem* operator<<(Telem const *p, const _v2<Telem> &v) {
    Telem *_p = (Telem*)p;
    *(_p++) = v.x;
    *(_p++) = v.y;
    *(_p++) = 0.0f;
    return _p;
}

typedef _v2<float> v2;

template<class Tv>
class dist_out {
    public:

    Tv p, v;
};

template<class Tv>
class accum_pt {
    public:

        Tv p, f;
        typename Tv::Telem m = 0;

        accum_pt(Tv p) : p(p) {}

        auto accum(Tv ps, Tv pd) {
            auto d = pd-ps;
            auto vf = d * d.rqlen(); // TODO: dist func
            f += vf;
            m -= (p-pd).vp(vf);
        }
};

template<class Tv>
class pt_sect { // TODO: pack / unpack ?
    public:

        Tv p, v;
        typename Tv::Telem l;

        pt_sect(Tv p, Tv v, typename Tv::Telem l)
            : p(p), v(v), l(l) {}

        Tv proc(Tv p2) {
            auto d = p2-p;
            auto l2 = v.dp(d);
            if (l2<0) return p;
            if (l2>l) return p+v*l;
            return p+v*l2;
        }

        void set_and(typename Tv::Telem a) {
            v = v2(cos(a), sin(a));
        } 
};
template<class Tv>
typename Tv::Telem* operator<<(typename Tv::Telem const *pp, pt_sect<Tv> &p) {
    auto p2 = p.p+p.v*p.l;
    return pp << p.p << p2;
}


template<class Tv>
class dir_line {
    public:
        Tv p;
        typename Tv::Telem a, l;

        dir_line(Tv p, typename Tv::Telem a, typename Tv::Telem l)
            : p(p), a(a), l(l) {}

        auto unpack() {
            return pt_sect<Tv>(p, v2(cos(a), sin(a)), l);
        }

        void apply(accum_pt<Tv> &acc) {
            p += acc.f*0.01f;
            a += acc.m*0.01f;
            if (a < M_PI) a += M_PI;
            if (a > M_PI) a -= M_PI;
        }
};

int main() {
    dir_line<v2> ld(v2(-0.1f, -0.0f), M_PI/4.1, 1.0f);

    //cout << ps.proc(v2(0.0f, 1.0f)) << endl;

    GLFWwindow *window = NULL;
    const GLubyte *renderer;
    const GLubyte *version;
    GLuint vao;
    GLuint vbo;
    /* geometry to use. these are 3 xyz points (9 floats total) to make a triangle */
    float points[12];// = { 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f };

    /* these are the strings of code for the shaders
    the vertex shader positions each vertex point */
    const char *vertex_shader = "#version 330\n"
        "in vec3 vp;"
        "void main () {"
        "  gl_Position = vec4 (vp, 1.0);"
        "}";
    /* the fragment shader colours each fragment (pixel-sized area of the
    triangle) */
    const char *fragment_shader = "#version 330\n"
        "out vec4 frag_colour;"
        "void main () {"
        "  frag_colour = vec4 (0.5, 0.0, 0.5, 1.0);"
        "}";
    /* GL shader objects for vertex and fragment shader [components] */
    GLuint vert_shader, frag_shader;
    /* GL shader programme object [combined, to link] */
    GLuint shader_programme;

    /* start GL context and O/S window using the GLFW helper library */
    if ( !glfwInit() ) {
        fprintf( stderr, "ERROR: could not start GLFW3\n" );
        return 1;
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    window = glfwCreateWindow( 640, 480, "Hello Triangle", NULL, NULL );
    if ( !window ) {
        fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent( window );
    /* start GLEW extension handler */
    glewExperimental = GL_TRUE;
    glewInit();

    /* get version info */
    renderer = glGetString( GL_RENDERER ); /* get renderer string */
    version = glGetString( GL_VERSION );     /* version as a string */
    printf( "Renderer: %s\n", renderer );
    printf( "OpenGL version supported %s\n", version );

    /* tell GL to only draw onto a pixel if the shape is closer to the viewer
    than anything already drawn at that pixel */
    glEnable( GL_DEPTH_TEST ); /* enable depth-testing */
    /* with LESS depth-testing interprets a smaller depth value as meaning "closer" */
    glDepthFunc( GL_LESS );
    /* a vertex buffer object (VBO) is created here. this stores an array of
    data on the graphics adapter's memory. in our case - the vertex points */
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, 9 * sizeof( GLfloat ), points, GL_STATIC_DRAW );

    /* the vertex array object (VAO) is a little descriptor that defines which
    data from vertex buffer objects should be used as input variables to vertex
    shaders. in our case - use our only VBO, and say 'every three floats is a
    variable' */
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    // "attribute #0 should be enabled when this vao is bound"
    glEnableVertexAttribArray( 0 );
    // this VBO is already bound, but it's a good habit to explicitly specify which
    // VBO's data the following
    // vertex attribute pointer refers to
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    // "attribute #0 is created from every 3 variables in the above buffer, of type
    // float (i.e. make me vec3s)"
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );

    /* here we copy the shader strings into GL shaders, and compile them. we
    then create an executable shader 'program' and attach both of the compiled
            shaders. we link this, which matches the outputs of the vertex shader to
    the inputs of the fragment shader, etc. and it is then ready to use */
    vert_shader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vert_shader, 1, &vertex_shader, NULL );
    glCompileShader( vert_shader );
    frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( frag_shader, 1, &fragment_shader, NULL );
    glCompileShader( frag_shader );
    shader_programme = glCreateProgram();
    glAttachShader( shader_programme, frag_shader );
    glAttachShader( shader_programme, vert_shader );
    glLinkProgram( shader_programme );

    /* this loop clears the drawing surface, then draws the geometry described
            by the VAO onto the drawing surface. we 'poll events' to see if the window
    was closed, etc. finally, we 'swap the buffers' which displays our drawing
            surface onto the view area. we use a double-buffering system which means
            that we have a 'currently displayed' surface, and 'currently being drawn'
            surface. hence the 'swap' idea. in a single-buffering system we would see
            stuff being drawn one-after-the-other */
    while ( !glfwWindowShouldClose( window ) ) {
        /* wipe the drawing surface clear */
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glUseProgram( shader_programme );
        glBindVertexArray( vao );
        /* draw points 0-3 from the currently bound VAO with current in-use shader */
        //points[0] += 0.001f;
        auto ps = ld.unpack();
        accum_pt<v2> ap(ps.p);
        ap.accum(v2(1.0f, 1.0f), ps.proc(v2(1.0f, 1.0f)));
        ap.accum(v2(-1.0f, 1.0f), ps.proc(v2(-1.0f, 1.0f)));
        ap.accum(v2(1.0f, -1.0f), ps.proc(v2(1.0f, -1.0f)));
        ap.accum(v2(-1.0f, -1.0f), ps.proc(v2(-1.0f, -1.0f)));
        ld.apply(ap);
        //ps.apply(ap);
        //cout << ld.a << endl;
        points << ps;// << ps.proc(v2(0.0f, 0.0f)) << v2(0.0f, 0.0f);
        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, 6 * sizeof( float ), points, GL_STATIC_DRAW );
        glDrawArrays( GL_LINES, 0, 2 );
        //glDrawArrays( GL_TRIANGLES, 0, 3 );
        /* update other events like input handling */
        glfwPollEvents();
        /* put the stuff we've been drawing onto the display */
        glfwSwapBuffers( window );
    }

    /* close GL context and any other GLFW resources */
    glfwTerminate();

    return 0;
}

