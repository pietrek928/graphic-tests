#ifndef __GL_OBJ_H_
#define __GL_OBJ_H_


#define GL_GLEXT_PROTOTYPES

#include <stdexcept>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

#include "utils.h"

template<const char *obj_name, void(*creator)(GLuint*), void(*destroyer)(GLuint*)>
class ObjectId {
    GLuint id = GL_FALSE;

    RARE_FUNC
    void create_error_handler() {
        throw std::runtime_error(((std::string)"Creating OpenGL ") + obj_name + " object failed ;/");
    }

    inline void _delete() {
        destroyer(&id);
        DBG_STREAM(globj) << obj_name << "(" << id << ") destroyed" << DBG_ENDL;
    }

    public:
        ObjectId(EmptyObject _) {
        }

        // TODO: bulk create and delete
        ObjectId() {
            creator(&id);
            DBG_STREAM(globj) << obj_name << "(" << id << ") created" << DBG_ENDL;
            if (unlikely(id == GL_FALSE))
                create_error_handler();
        }

        ObjectId(GLuint id)
            : id(id) {
        }

        ObjectId(ObjectId &&o)
            : id(o.id) {
            o.clear_no_destroy();
        }

        ~ObjectId() {
            if (likely(id != GL_FALSE)) {
                _delete();
            }
        }

        ObjectId &operator=(ObjectId &&o) {
            clear();
            id = o.id;
            o.clear_no_destroy();
            return *this;
        }

        void clear() {
            if (likely(id != GL_FALSE)) {
                _delete();
                id = GL_FALSE;
            }
        }

        void clear_no_destroy() {
            id = GL_FALSE;
        }

        auto get_glid() const {
            return id;
        }

        operator GLuint() const {
            return id;
        }
};

constexpr char ProgramObject_name[] = "Program";
using ProgramObject = ObjectId<
    ProgramObject_name,
    [](GLuint *id) {*id = glCreateProgram();},
    [](GLuint *id) {glDeleteProgram(*id);}
>;
constexpr char VertexShaderObject_name[] = "VertexShader";
using VertexShaderObject = ObjectId<
    VertexShaderObject_name,
    [](GLuint *id) {*id = glCreateShader(GL_VERTEX_SHADER);},
    [](GLuint *id) {glDeleteShader(*id);}
>;
constexpr char FragmentShaderObject_name[] = "FragmentShader";
using FragmentShaderObject = ObjectId<
    FragmentShaderObject_name,
    [](GLuint *id) {*id = glCreateShader(GL_FRAGMENT_SHADER);},
    [](GLuint *id) {glDeleteShader(*id);}
>;

#define DECLARE_GL_OBJECT(OBJECT_NAME, GL_NAME)         \
constexpr char CONCAT3(__, OBJECT_NAME, Object_name)[] = \
    STRINGIFY(GL_NAME);                                   \
using CONCAT(OBJECT_NAME, Object) = ObjectId<              \
    CONCAT3(__, OBJECT_NAME, Object_name),                  \
    [](GLuint *id) {CONCAT3(glGen, GL_NAME, s)(1, id);},     \
    [](GLuint *id) {CONCAT3(glDelete, GL_NAME, s)(1, id);}    \
>;                                                             \

DECLARE_GL_OBJECT(VertexArray, VertexArray)
DECLARE_GL_OBJECT(Buffer, Buffer)
DECLARE_GL_OBJECT(FrameBuffer, Framebuffer)

DECLARE_GL_OBJECT(Texture, Texture)
DECLARE_GL_OBJECT(RenderBuffer, Renderbuffer)

#undef DECLARE_GL_OBJECT

// TODO: debug only
#define glCheckError() { \
  auto e = glGetError();  \
   if (e != GL_NO_ERROR) { \
      DBG_STREAM(gl) << __FILE__ "(" __func__  "):" __LINE__ "error code: " << e << endl; \
   } \
}

#endif /* __GL_OBJ_H_ */

