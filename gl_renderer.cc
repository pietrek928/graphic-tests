#include <iostream>

#include "gl_obj.h"

#include <boost/python.hpp>

#include "utils.h"
#include "obj.h"

template<class T>
constexpr GLuint GLType;
template<>
constexpr GLuint GLType<float> = GL_FLOAT;
template<>
constexpr GLuint GLType<double> = GL_DOUBLE;
template<>
constexpr GLuint GLType<int8_t> = GL_BYTE;
template<>
constexpr GLuint GLType<uint8_t> = GL_UNSIGNED_BYTE;
template<>
constexpr GLuint GLType<int16_t> = GL_SHORT;
template<>
constexpr GLuint GLType<uint16_t> = GL_UNSIGNED_SHORT;
template<>
constexpr GLuint GLType<int32_t> = GL_INT;
template<>
constexpr GLuint GLType<uint32_t> = GL_UNSIGNED_INT;

template<class T>
void glSetUniform(GLint loc, T v);
template<>
void glSetUniform(GLint loc, GLfloat v) {glUniform1f(loc, v);}
template<>
void glSetUniform(GLint loc, _v2<GLfloat> v) {glUniform2f(loc, v.x, v.y);}
template<>
void glSetUniform(GLint loc, GLint v) {glUniform1i(loc, v);}
template<>
void glSetUniform(GLint loc, _v2<GLint> v) {glUniform2i(loc, v.x, v.y);}
template<>
void glSetUniform(GLint loc, GLuint v) {glUniform1ui(loc, v);}
template<>
void glSetUniform(GLint loc, _v2<GLuint> v) {glUniform2ui(loc, v.x, v.y);}

auto glNumberSize(GLuint number_format) {
    switch(number_format) {
        case GL_UNSIGNED_BYTE:
            return 1;
        case GL_BYTE:
            return 1;
        case GL_UNSIGNED_SHORT:
            return 2;
        case GL_SHORT:
            return 2;
        case GL_UNSIGNED_INT:
            return 4;
        case GL_INT:
            return 4;
        case GL_HALF_FLOAT:
            return 2;
        case GL_FLOAT:
            return 4;
        default:
            return 0;
    }
}

auto glGetColorFormatSize(GLuint color_format) {
    switch(color_format) {
        case GL_UNSIGNED_BYTE_3_3_2:
            return 1;
        case  GL_UNSIGNED_BYTE_2_3_3_REV:
            return 1;
        case GL_UNSIGNED_SHORT_5_6_5:
            return 2;
        case GL_UNSIGNED_SHORT_5_6_5_REV:
            return 2;
        case GL_UNSIGNED_SHORT_4_4_4_4:
            return 2;
        case GL_UNSIGNED_SHORT_4_4_4_4_REV:
            return 2;
        case GL_UNSIGNED_SHORT_5_5_5_1:
            return 2;
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
            return 2;
        case GL_UNSIGNED_INT_8_8_8_8:
            return 4;
        case GL_UNSIGNED_INT_8_8_8_8_REV:
            return 4;
        case GL_UNSIGNED_INT_10_10_10_2:
            return 4;
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            return 4;
        default:
            return 0;
    }
}

auto glColorChannelsCount(GLuint color_format) {
    switch(color_format) {
        case GL_RED:
            return 1;
        case GL_RG:
            return 2;
        case GL_RGB:
            return 3;
        case GL_BGR:
            return 3;
        case GL_RGBA:
            return 4;
        case  GL_BGRA:
            return 4;
        case  GL_RED_INTEGER:
            return 1;
        case GL_RG_INTEGER:
            return 2;
        case GL_RGB_INTEGER:
            return 3;
        case GL_BGR_INTEGER:
            return 3;
        case GL_RGBA_INTEGER:
            return 4;
        case GL_BGRA_INTEGER:
            return 4;
        case GL_STENCIL_INDEX:
            return 1;
        case GL_DEPTH_COMPONENT:
            return 1;
        case GL_DEPTH_STENCIL:
            return 1;
        default:
            return 0;
    }
}

template<class T>
class DataBufferConst {
    const T *data = NULL;
    size_t vsize;

    public:

    DataBufferConst() {
    }

    DataBufferConst(const T* data, size_t vsize)
            : data(data), vsize(vsize) {
    }

    DataBufferConst(std::initializer_list<const T> src_data)
        : data(src_data.begin()), vsize(vsize) {
        std::copy(src_data.begin(), src_data.end(), data);
    }

    inline auto size() {
        return vsize;
    }

    inline auto begin() {
        return data;
    }
};

 //TODO: align data
//template<size_t align_l = 16>
class DataBuffer {
    public:
    byte *data = NULL;
    size_t vsize;

    RARE_FUNC
    void static handle_alloc_error() {
        throw std::bad_alloc();
    }

    inline static byte *alloc_data(size_t s) {
        byte *r = (byte*)malloc(s);
        if (unlikely(!r)) {
            handle_alloc_error();
        }
        return r;
    }

    inline static byte *realloc_data(byte *data, size_t s) {
        byte *r = (byte*)realloc(data, s);
        if (unlikely(!r)) {
            handle_alloc_error();
        }
        return r;
    }

    inline static void free_data(byte* &data) {
        if (data) {
            free(data);
            data = NULL;
        }
    }

public:

    DataBuffer(EmptyObject _) {
    }

    DataBuffer(size_t vsize = 9)
            : vsize(vsize), data(alloc_data(vsize)) {
    }

    template<class T>
    DataBuffer(std::initializer_list<T> src_data)
        : vsize((byte*)src_data.end() - (byte*)src_data.begin()),
          data(alloc_data((byte*)src_data.end() - (byte*)src_data.begin())) {
        std::copy((T*)src_data.begin(), (T*)src_data.end(), (T*)data);
    }

    DataBuffer(DataBuffer &&b) {
        data = b.data;
        vsize = b.vsize;
        b.data = NULL;
    }

    DataBuffer &operator=(DataBuffer &&b) {
        clear();
        data = b.data;
        vsize = b.vsize;
        b.data = NULL;
        return *this;
    }

    void resize(size_t n) {
        if (likely(data)) {
            data = realloc_data(data, n);
        } else {
            data = alloc_data(n);
        }
        vsize = n;
    }

    void alloc_new() {
        if (likely(data)) {
            auto new_data = alloc_data(vsize);
            std::copy(data, data + vsize, new_data);
            free_data(data);
            data = new_data;
        }
    }

    /*auto &operator=(PointStream &&p) {
        clear();
        data = p.data;
        p.data = NULL;
        return *this;
    }*/

    inline auto size() {
        return vsize;
    }

    /*inline void prepare(int n) {
        n *= 3;

        auto l = size();
        if (l + n <= vsize) {
            return;
        }
        auto new_size = vsize;
        do {
            new_size = vsize * 3 / 2;
        } while (new_size < l + n);

        dst_v = (float *) realloc(
                dst_v, new_size * sizeof(float)
        );
        vsize = new_size;
        _ptr = dst_v + l;
    }

    auto &operator<<(float v) {
        prepare(1);
        put(v);
        return *this;
    }

    inline auto &operator<<(v2 v) {
        prepare(3);
        put(v.x);
        put(v.y);
        put(0.0f);
        return *this;
    }*/

    inline auto begin() {
        return data;
    }

    /*void show() {
        auto l = size();
        for (int i = 0; i < l; i++) {
            std::cout << dst_v[i] << " ";
        }
        std::cout << std::endl;
    }*/

    void clear() {
        free_data(data);
    }

    ~DataBuffer() {
        clear();
    }
};

template<int N, class... Ts>
using NthTypeOf =
    typename std::tuple_element<N, std::tuple<Ts...>>::type;

NthTypeOf<0, v2, v2>::Telem aaa = 0.0f;

struct _pervertex_size_calc {
    size_t s = 0;
    template<class Tv>
    inline void process(Tv &v) {
        s += Tv::len * sizeof(typename Tv::Telem);
    }
};



struct _array_size_calc {
    size_t s = 0;
    size_t n;
    _array_size_calc(size_t n)
        : n(n) {
    }
    template<class Tv>
    inline void process(Tv &v) { // TODO: align
        s += (Tv::len * sizeof(Tv::Telem)) * n;
    }
};

struct _array_setup {
    size_t esize;
    size_t pos = 0;
    GLuint i = 0;

    inline _array_setup(size_t esize)
        : esize(esize) {
    }

    template<class Tv>
    inline void process(Tv &v) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, Tv::len, GLType<typename Tv::Telem>, GL_FALSE, esize, (void *) pos);

        i++;
        pos += sizeof(typename Tv::Telem) * Tv::len;
    }
};

template<GLuint draw_mode, class... Tv>
class VertexArray {
    VertexArrayObject array;
    BufferObject buffer;
    DataBuffer data;

    using Ttuple = std::tuple<Tv...>;

    inline void setup_arrays() {
        _array_setup setup(pervertex_size());
        for_each(setup, Ttuple());
    }

    inline size_t pervertex_size() {
        _pervertex_size_calc accum;
        for_each(accum, Ttuple());
        return accum.s;
    }

public:
    VertexArray(EmptyObject _)
        : array(_),
          buffer(_),
          data(3) {
    } 

    VertexArray(int size = 3) : data(size) {
    }

    //VertexArray(std::initializer_list<NthTypeOf<0, Tv...>::Telem> src_data)
    VertexArray(std::initializer_list<float> src_data)
        : data(src_data) {
            // TODO: assert
    }

    VertexArray(VertexArray &&a)
        : array(std::move(a.array)),
          buffer(std::move(a.buffer)),
          data(std::move(a.data)) {
    }

    VertexArray &operator=(VertexArray &&a) {
        array = std::move(a.array);
        buffer = std::move(a.buffer);
        data = std::move(a.data);
        return *this;
    }

    void attach_data_static() {
        glBindVertexArray(array);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(
            GL_ARRAY_BUFFER, data.size(), data.begin(), GL_STATIC_DRAW
        );
        setup_arrays();
    }

    void attach_data_dynamic() {
        glBindVertexArray(array);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(
            GL_ARRAY_BUFFER, data.size(), data.begin(), GL_DYNAMIC_DRAW
        );
        setup_arrays();
    }

    void draw() {
        glBindVertexArray(array);
        //DBG_STREAM(ppppppp) << ((float*)data.data)[0] << " " << ((float*)data.data)[1] << DBG_ENDL;
        glDrawArrays(draw_mode, 0, data.size() / pervertex_size());
    }

    auto &stream() {
        return data;
    }
};

template<class ShaderClass>
class ShaderTemplate {
    ShaderClass shader;

    void handle_compile_error() {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        std::vector<char> buffer(log_len + 1, 0);
        glGetShaderInfoLog(shader, log_len, NULL, &*buffer.begin());

        throw std::runtime_error(
                ((std::string) "Shader initialization error:\n") + std::string(buffer.begin(), buffer.end()));
    }

public:
    ShaderTemplate(EmptyObject _)
            : shader(_) {
    }

    ShaderTemplate(const char *code) {
        auto _code = code;
        glShaderSource(shader, 1, &_code, NULL);
        glCompileShader(shader);

        int status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (unlikely(status == GL_FALSE))
            handle_compile_error();
    }

    ShaderTemplate(ShaderTemplate &&s)
            : shader(std::move(s.shader)) {
    }

    ShaderTemplate &operator=(ShaderTemplate &&s) {
        s = std::move(s.shader);
        return *this;
    }

    operator GLuint() {
        return shader;
    }
};

using VertexShader = ShaderTemplate<VertexShaderObject>;
using FragmentShader = ShaderTemplate<FragmentShaderObject>;

class Program {
    ProgramObject program;

    void handle_linking_error() {
        int log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

        std::vector<char> buffer(log_len + 1, 0);
        glGetProgramInfoLog(program, log_len, NULL, &*buffer.begin());

        throw std::runtime_error(
                ((std::string) "Linking OpenGL program error:\n") + std::string(buffer.begin(), buffer.end()));
    }

public:
    Program(EmptyObject _)
            : program(_) {
    }

    Program(Program &&p)
            : program(std::move(p.program)) {
    }

    Program &operator=(Program &&p) {
        program = std::move(p.program);
        return *this;
    }

    Program(
            VertexShader &vertex,
            FragmentShader &fragment) {

        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        GLint program_linked;
        glGetProgramiv(program, GL_LINK_STATUS, &program_linked);

        if (unlikely(program_linked != GL_TRUE))
            handle_linking_error();
    }

    operator GLuint() {
        return program;
    }

    void use() {
        glUseProgram(program);
    }

    auto const var_location(const char *name) const {
        glGetUniformLocation(program, name);
    }

};

class TexturePixelFormat {

    static auto gl

    public:

    GLuint color_format;
    GLuint number_format;

    TexturePixelFormat(GLuint color_format, GLuint number_format)
    : color_format(color_format), number_format(number_format) {
    }

    GLuint get_size_bits() {
        switch(color_format) {
            auto format_size = glGetColorFormatSize(number_format);
            if (likely(!format_size)) {
                return glColorChannelsCount(color_format) * glNumberSize(number_format);
            }
            return format_size;
        }
    }
}

class Texture {
    TextureObject tex;
    GLuint width, height;
    bool use_mipmap = false

    void init_params() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // TODO: mirror ?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                use_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR); // TODO: generate later
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    Texture(EmptyObject _)
        : tex(_) {}

    Texture(const void *data, GLuint width, GLuint height)
        : width(width), height(height) {
        bind();
        init_params();
        init_data(data);
    }

#ifdef OPENCV_CORE_BASE_HPP
    //
#endif /* OPENCV_CORE_BASE_HPP */

    Texture(XConfig &c, const void *data, GLuint width, GLuint height)
        : width(width), height(height), use_mipmap(c.get("use_mipmap", false)) {
        bind();
        init_params();
        init_data(data);
    }

    Texture(Texture &&t) {
        tex = std::move(t.tex);
        width = t.width;
        height = t.height;
    }

    Texture &operator=(Texture &&t) {
        tex = std::move(t.tex);
        width = t.width;
        height = t.height;
    }

    void bind() {
        glBindTexture(GL_TEXTURE_2D, tex);
    }

    void select(GLuint slot_num) {
        // TODO: debug assert slot num ?
        glActiveTexture(GL_TEXTURE0 + slot_num);
        bind();
    }

    void init_data(const void *data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        if (use_mipmap) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    void update_data(const void *data) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        if (use_mipmap) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
};

class NormalBuffer {
    FrameBufferObject framebuffer;
    RenderBufferObject rbo_color, rbo_depth;
    TextureObject screenTexture;
    Program screen_program;

    v2i buffer_size;
    bool use_depth = false; //true; !!!!!!!!!! ???????????????

    VertexArray<GL_TRIANGLES, v2, v2> quad_arr;

    void init_buffers() {
       glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
       glBindTexture(GL_TEXTURE_2D, screenTexture);

       /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // */
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // */

       glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

       //NULL means reserve texture memory, but texels are undefined
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer_size.x, buffer_size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
       glGenerateTextureMipmap(screenTexture);
       //-------------------------
       //Attach 2D texture to this FBO
       glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
       glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, buffer_size.x, buffer_size.y);
       glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_depth);

       if (use_depth) {
           glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
           glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, buffer_size.x, buffer_size.y);
           glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
       }

        if (unlikely(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE))
            throw std::runtime_error("Framebuffer is not complete!");

       glBindTexture(GL_TEXTURE_2D, 0);
       glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

public:
    NormalBuffer(EmptyObject _) :
        framebuffer(_),
        screenTexture(_),
        rbo_color(_),
        rbo_depth(_),
        quad_arr(_),
        screen_program(_) {
    }

    NormalBuffer(
            v2i buffer_size
    ) : buffer_size(buffer_size),
    quad_arr({
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f,
    }),
    screen_program(empty) {
        FragmentShader fragment(
                "#version 330" ENDL
                "in vec2 vertTexCoord;"
                "out vec3 color;"
                "uniform sampler2D myTexture;"
                "void main() {"
                    "color = texture(myTexture, vertTexCoord).rgb;"
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

        screen_program = Program(vertex, fragment);

        init_buffers();
        quad_arr.attach_data_static();
    }

    NormalBuffer(NormalBuffer &&b)
        : framebuffer(std::move(b.framebuffer)),
          screenTexture(std::move(screenTexture)),
          rbo_color(std::move(b.rbo_color)),
          rbo_depth(std::move(b.rbo_depth)),
          quad_arr(std::move(b.quad_arr)),
          buffer_size(buffer_size),
          screen_program(std::move(screen_program)) {
    }

    NormalBuffer &operator=(NormalBuffer &&b) {
        framebuffer = std::move(b.framebuffer);
        screenTexture = std::move(screenTexture);
        rbo_color = std::move(b.rbo_color);
        rbo_depth = std::move(b.rbo_depth);
        quad_arr = std::move(b.quad_arr);
        buffer_size = b.buffer_size;
        screen_program = std::move(b.screen_program);
        return *this;
    }

    auto size() {
        return buffer_size;
    }

    void startDrawing() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, buffer_size.x, buffer_size.y);
        if (use_depth) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glClearColor(0.5f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void endDrawing(GLint prev_buffer = 0) {
        glFlush();
        glGenerateTextureMipmap(screenTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, prev_buffer);
    }

    void display() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        screen_program.use();
        quad_arr.draw();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

#if 0
class MSAABuffer : public NormalBuffer {
    FrameBufferObject msaa_fbo;
    RenderBufferObject msaa_rbo_color, msaa_rbo_depth;
    GLuint nsamples;

    void init_buffers() {
        NormalBuffer::init_buffers();
        glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);

        glBindRenderbuffer(GL_RENDERBUFFER, msaa_rbo_color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, nsamples, GL_RGBA8, buffer_size.x, buffer_size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_rbo_color);

        glBindRenderbuffer(GL_RENDERBUFFER, msaa_rbo_depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, nsamples, GL_DEPTH_COMPONENT, buffer_size.x, buffer_size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaa_rbo_depth);

        if (unlikely(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE))
            throw std::runtime_error("Framebuffer is not complete!");
       glBindTexture(GL_TEXTURE_2D, 0);
       glBindRenderbuffer(GL_RENDERBUFFER, 0);

        //glBindFramebuffer(GL_FRAMEBUFFER, 3);
#if 0
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // create a multisampled color attachment texture
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, buffer_size.x, buffer_size.y, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                textureColorBufferMultiSampled, 0
        );

        // create a (also multisampled) renderbuffer object for depth and stencil attachments
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_DEPTH24_STENCIL8, buffer_size.x, buffer_size.y);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (unlikely(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE))
            throw std::runtime_error("FrameBuffer is not complete!");
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // configure second post-processing framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

        // create a color attachment texture
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer_size.x, buffer_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0
        );  // we only need a color buffer

        if (unlikely(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE))
            throw std::runtime_error("Intermediate framebuffer is not complete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
    }

public:
    MSAABuffer(EmptyObject _) :
            framebuffer(_),
            intermediateFBO(_),
            textureColorBufferMultiSampled(_),
            screenTexture(_),
            rbo(_),
            rbo_depth(_),
            quad_arr(_),
            screen_program(_) {
    }

    MSAABuffer(
            v2i buffer_size
    ) : buffer_size(buffer_size),
    quad_arr({
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f,
    }),
    screen_program(empty) {
        FragmentShader fragment(
                "#version 330" ENDL
                "in vec2 vertTexCoord;"
                "out vec3 color;"
                "uniform sampler2D myTexture;"
                "void main() {"
                    "color = texture(myTexture, vertTexCoord).rgb;"
                    //"color = vec3(vertTexCoord, vertTexCoord.x * vertTexCoord.y);"
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

        screen_program = Program(vertex, fragment);

        init_buffers();
        quad_arr.attach_data_static();
    }

    MSAABuffer(MSAABuffer &&b)
        : framebuffer(std::move(b.framebuffer)),
          intermediateFBO(std::move(b.intermediateFBO)),
          textureColorBufferMultiSampled(std::move(b.textureColorBufferMultiSampled)),
          screenTexture(std::move(screenTexture)),
          rbo(std::move(b.rbo)),
          rbo_depth(std::move(b.rbo_depth)),
          quad_arr(std::move(b.quad_arr)),
          buffer_size(buffer_size),
          screen_program(std::move(screen_program)) {
    }

    MSAABuffer &operator=(MSAABuffer &&b) {
        framebuffer = std::move(b.framebuffer);
        intermediateFBO = std::move(b.intermediateFBO);
        textureColorBufferMultiSampled = std::move(b.textureColorBufferMultiSampled);
        screenTexture = std::move(screenTexture);
        rbo = std::move(b.rbo);
        rbo_depth = std::move(b.rbo_depth);
        quad_arr = std::move(b.quad_arr);
        buffer_size = b.buffer_size;
        screen_program = std::move(b.screen_program);
        return *this;
    }

    auto size() {
        return buffer_size;
    }

    void startDrawing() {
        //GLint drawFboId = 0, readFboId = 0;
        //glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
        //glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
        //DBG_STREAM(aaaaaaaaa) << drawFboId << " " << readFboId << DBG_ENDL;

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, buffer_size.x, buffer_size.y);
        glClearColor(0.5f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void endDrawing(GLint prev_buffer = 0) {
        glFlush();
        glBindFramebuffer(GL_FRAMEBUFFER, prev_buffer);
    }

    void display() {
        //glFlush();
        //GLubyte pixels[4*4*4];
        //glReadPixels(0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        //DBG_STREAM(eeee) << (int)pixels[0] << " " << (int)pixels[1] << " " << (int)pixels[2] << DBG_ENDL;
        //DBG_STREAM(eeee) << (int)pixels[3] << " " << (int)pixels[4] << " " << (int)pixels[5] << DBG_ENDL;
        //DBG_STREAM(eeee) << (int)pixels[3+32] << " " << (int)pixels[4+32] << " " << (int)pixels[5+32] << DBG_ENDL;
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
        //glBlitFramebuffer(
            //0, 0, buffer_size.x, buffer_size.y,
            //0, 0, buffer_size.x, buffer_size.y,
            //GL_COLOR_BUFFER_BIT, GL_LINEAR
        //);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glBindFramebuffer(GL_FRAMEBUFFER, 3);

        //glViewport(0, 0, buffer_size.x, buffer_size.y);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        screen_program.use();
        quad_arr.draw();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};
#endif

namespace py = boost::python;

GLfloat verts[] =
        {
                +0.7, +0.7, +0.7,
                -0.7, 0.7, 0.7,
                -0.7, +0.7, -0.7,

                0.7, +0.7, -0.7,
                +0.7, +0.7, +0.7,
                -0.7, +0.7, -0.7,

                -0.7, -0.7, +0.7,
                +0.7, +0.7, +0.7,
                -0.7, 0.7, 0.7,

                +0.7, +0.7, +0.7,
                -0.7, -0.7, +0.7,
                0.7, -0.7, 0.7,

                +0.7, -0.7, -0.7,
                -0.7, -0.7, -0.7,
                0.7, +0.7, -0.7,

                -0.7, +0.7, -0.7,
                0.7, +0.7, -0.7,
                -0.7, -0.7, -0.7,


                -0.7, -0.7, +0.7,
                -0.7, -0.7, -0.7,
                0.7, -0.7, 0.7,


                +0.7, -0.7, -0.7,

                0.7, -0.7, 0.7,

                -0.7, -0.7, -0.7,

                0.7, +0.7, -0.7,
                +0.7, +0.7, +0.7,
                +0.7, -0.7, -0.7,

                0.7, -0.7, -0.7,
                +0.7, +0.7, +0.7,


                0.7, -0.7, 0.7,


                -0.7, -0.7, -0.7,
                -0.7, 0.7, 0.7,
                -0.7, +0.7, -0.7,


                -0.7, -0.7, +0.7,
                -0.7, 0.7, 0.7,
                -0.7, -0.7, -0.7

        };
GLuint gl_vao, gl_buffer, gl_program;
GLint mvp;

#if 0

class _GLRenderer {
    py::object gl_area_object;
    GtkGLArea *gl_area;

    Program program = empty;
    VertexArray<GL_LINES, v2> pt_buf = empty;
    MSAABuffer msaa = empty;

    v2i size = v2i(128, 128);

public:
    _GLRenderer(py::object gl_area_object)
            : gl_area_object(gl_area_object) {
        PyObject *pyobj = gl_area_object.ptr();

        GObject *obj = pygobject_get(pyobj);
        gl_area = GTK_GL_AREA(obj);
        //gtk_gl_area_set_auto_render(gl_area, true);

        g_signal_connect(gl_area, "realize", GCBK(&_GLRenderer::realize), (void *) this);
        g_signal_connect(gl_area, "unrealize", GCBK(&_GLRenderer::unrealize), (void *) this);
        g_signal_connect(gl_area, "render", GCBK(&_GLRenderer::render), (void *) this);
        g_signal_connect(gl_area, "size-allocate", GCBK(&_GLRenderer::resize), (void *) this);
    }

    gboolean render(GtkGLArea *area, GdkGLContext *context) {
        glDisable(GL_DEPTH_TEST);

        GLint drawFboId = 0, readFboId = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
        DBG_STREAM(aaaaaaaaa) << drawFboId << " " << readFboId << DBG_ENDL;

        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //std::cout << "GLArea render" << std::endl;
        //glLoadIdentity();
        //glMatrixMode(GL_MODELVIEW);
        glClearColor(0.3f, 0.3f, 0.3f, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        msaa.startDrawing();
        program.use();

        //glColor3f(1.0f, 1.0f, 0.0f);
        //glBegin(GL_TRIANGLES);
        //glVertex2f( 0.0f, 1.0f);				// Top
        //glVertex2f(-1.0f, -1.0f);				// Bottom Left
        //glVertex2f( 1.0f, -1.0f);				// Bottom Right
        //glEnd();							// Finished Drawing The Triangle
//glBegin(GL_LINES);
//glVertex2f( 0.0f, 0.0f);
//glVertex2f(-1.0f,-1.0f);
//glEnd();

        /*ObjectContainer cont;
        cont.pt_cont.put(v2(-100.0, -100.0));
        cont.pt_cont.put(v2(100.0, 100.0));
        cont.cont_Sect.emplace_back(0, 1);
        //glColor3f(1.0, 0.0, 0.0);
        cont.render();*/

        /*glBindVertexArray(gl_vao);
        glDrawArrays(GL_LINES,0,36 ); // */
        //glLineWidth(5.0f);
        pt_buf.draw();
        glUseProgram(0);

        msaa.endDrawing();

        glFlush();

        if (size != msaa.size()) {
            msaa = MSAABuffer(size);
        }

        gtk_gl_area_queue_render(area);
        return TRUE;
    }

    gboolean realize(GtkGLArea *area) {
        gtk_gl_area_make_current(area);
        DBG_STREAM(gtk) << "GLArea realize" << DBG_ENDL;

        //glDisable(GL_DEPTH_TEST);
        //glEnable(GL_BLEND);

        pt_buf = {
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, -1.0f
        };

        //glEnable(GL_MULTISAMPLE);
        //glSampleCoverage(0.1, 0);

        FragmentShader fragment(
            "#version 330" ENDL
            "out vec4 outputColor;"
            "void main() {"
            "float lerpVal = gl_FragCoord.y / 500.0f;"
            "outputColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2f, 0.2f, 1.0f), lerpVal);"
            "}"
        );

        VertexShader vertex(
            "#version 330" ENDL
            "layout(location = 0) in vec4 position;"
            "void main() {"
            "gl_Position = position;"
            "}"
        );

        program = Program(vertex, fragment);

        msaa = MSAABuffer(size);

        /*pt_buf = VertexArray<v2, GL_TRIANGLES>();
        pt_buf.stream() << v2(0.0f, 0.6f) << v2(1.0f, 1.0f);
        pt_buf.stream().show();
        pt_buf.attach_data_static();*/

        /*glGenVertexArrays(1, &gl_vao);
        glBindVertexArray(gl_vao);

        glGenBuffers(1, &gl_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0); // */

        return TRUE;
    }

    gboolean unrealize(GtkGLArea *area) {
        DBG_STREAM(gtk) << "GLArea unrealize" << DBG_ENDL;
        gtk_gl_area_make_current(area);
        // TODO: destroy objects

        return TRUE;
    }

    gboolean resize(GtkGLArea *area, GdkRectangle *allocation) {
        size = v2i(allocation->width, allocation->height);
        DBG_STREAM(gtk) << "GLArea resize " << size << DBG_ENDL;
        return TRUE;
    }

    auto get_area() {
        return gl_area_object;
    }
};


BOOST_PYTHON_MODULE(gl_renderer) {
    py::class_<_GLRenderer, boost::noncopyable>(
        "_GLRenderer", py::init<py::object>())
        .add_property("area", &_GLRenderer::get_area)
    ;
}
#endif

