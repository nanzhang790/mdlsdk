/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/example_execution_glsl.cpp
//
// Introduces the execution of generated code for compiled materials for
// the GLSL backend and shows how to manually bake a material
// sub-expression to a texture.

#include <iomanip>
#include <iostream>

#include <string>
#include <vector>

#include <mi/mdl_sdk.h>
#include "example_shared.h"
#include "example_glsl_shared.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


namespace{

// This selects SSBO (Shader Storage Buffer Objects) mode for passing uniforms and MDL const data.
// Should not be disabled unless you only use materials with very small const data.
// In this example, this would only apply to execution_material_2, because the others are using
// lookup tables for noise functions.
#ifdef MI_PLATFORM_MACOSX
    const int MAX_MATERIALS = 16;
    const int MAX_TEXTURES = 16;
#else
    //#define USE_SSBO
    const int MAX_MATERIALS = 64;
    const int MAX_TEXTURES = 32;
#endif

// If defined, the GLSL backend will remap these functions
//   float ::base::perlin_noise(float4 pos)
//   float ::base::mi_noise(float3 pos)
//   float ::base::mi_noise(int3 pos)
//   ::base::worley_return ::base::worley_noise(float3 pos, float jitter, int metric)
//
// to lut-free alternatives. When enabled, you can avoid to set the USE_SSBO define for this 
// example.
#define REMAP_NOISE_FUNCTIONS

inline bool use_ssbo() {
#if defined(USE_SSBO)
    return true;
#else
    return false;
#endif
}


// Enable this to dump the generated GLSL code to stdout.
#define DUMP_GLSL

char const* vertex_shader_filename   = "example_execution_glsl.vert";
char const* fragment_shader_filename = "example_execution_glsl.frag";

// Command line options structure.
struct Options {
    // The pattern number representing the combination of materials to display.
    unsigned int material_pattern;

    // The resolution of the display / image.
    unsigned int res_x, res_y;

    // If true, no interactive display will be used.
    bool no_window;

    // An result output file name for non-interactive mode.
    std::string outputfile;

    // The constructor.
    Options()
        : no_window(false)
        , outputfile("output.png")
        , material_pattern(7)
        , res_x(1024)
        , res_y(768)
    {
    }
};

// Struct representing a vertex of a scene object.
struct Vertex {
    mi::Float32_3_struct position;
    mi::Float32_2_struct tex_coord;
};

// Context structure for window callback functions.
struct Window_context
{
    // A number from 1 to 7 specifying the material pattern to display.
    unsigned material_pattern;
};

Window_context window_context = { 0 };


// GLFW callback handler for keyboard inputs.
static void handle_key(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    // Handle key press events
    if (action == GLFW_PRESS) {
        // Map keypad numbers to normal numbers
        if (GLFW_KEY_KP_0 <= key && key <= GLFW_KEY_KP_9) {
            key += GLFW_KEY_0 - GLFW_KEY_KP_0;
        }

        switch (key) {
        // Escape closes the window
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        // Numbers 1 - 7 select the different material patterns
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_4:
        case GLFW_KEY_5:
        case GLFW_KEY_6:
        case GLFW_KEY_7:
        {
            auto ctx = (Window_context*)glfwGetWindowUserPointer(window);
            ctx->material_pattern = key - GLFW_KEY_0;
            break;
        }

        default:
            break;
        }
    }
}

// GLFW callback handler for framebuffer resize events (when window size or resolution changes).
void handle_framebuffer_size(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

//------------------------------------------------------------------------------
//
// OpenGL code
//
//------------------------------------------------------------------------------

// Initialize OpenGL and create a window with an associated OpenGL context.
static GLFWwindow *init_opengl(Options const &options)
{
    // Initialize GLFW
    check_success(glfwInit());

    if (use_ssbo()) {
        // SSBO requires GLSL 4.30
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    }
    else {
        // else GLSL 3.30 is sufficient
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Hide window in no-window mode
    if (options.no_window)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Create an OpenGL window and a context
    GLFWwindow *window = glfwCreateWindow(options.res_x, options.res_y,
        "MDL SDK GLSL Execution Example - Switch pattern with keys 1 - 7", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error creating OpenGL window!" << std::endl;
        terminate();
    }

    // Enable VSync
    glfwSwapInterval(1);

    // Callbacks
    window_context.material_pattern = options.material_pattern;
    glfwSetWindowUserPointer(window, &window_context);
    glfwSetKeyCallback(window, handle_key);
    glfwSetFramebufferSizeCallback(window, handle_framebuffer_size);

    // Attach context to window
    glfwMakeContextCurrent(window);

    // Initialize GLEW to get OpenGL extensions
    const GLenum res = glewInit();
    if (res != GLEW_OK) {
        std::cerr << "GLEW error: " << glewGetErrorString(res) << std::endl;
        terminate();
    }

    check_gl_success();

    return window;
}

// Generate GLSL source code for a function executing an MDL subexpression function
// selected by a given id.
static std::string generate_glsl_switch_func(
    const mi::base::Handle<const mi::neuraylib::ITarget_code>& target_code)
{
    // Note: The "State" struct must be in sync with the struct in example_execution_glsl.frag and
    //       the code generated by the MDL SDK (see dumped code when enabling DUMP_GLSL).

    std::string src =
        "#version 330 core\n"
        "struct State {\n"
        "    vec3 normal;\n"
        "    vec3 geometry_normal;\n"
        "    float animation_time;\n"
        "    vec3[1] texture_tangent_u;\n"
        "    vec3[1] texture_tangent_v;\n"
        "};\n"
        "\n"
        "uint get_mdl_num_mat_subexprs() { return " +
        to_string(target_code->get_callable_function_count()) +
        "u; }\n"
        "\n";

    std::string switch_func =
        "vec3 mdl_mat_subexpr(uint id, State state) {\n"
        "    switch(id) {\n";

    // Create one switch case for each callable function in the target code
    for (size_t i = 0, num_target_codes = target_code->get_callable_function_count();
          i < num_target_codes;
          ++i)
    {
        std::string func_name(target_code->get_callable_function(i));

        // Add prototype declaration
        src += target_code->get_callable_function_prototype(
            i, mi::neuraylib::ITarget_code::SL_GLSL);
        src += '\n';

        switch_func += "        case " + to_string(i) + "u: return " + func_name + "(state);\n";
    }

    switch_func +=
        "        default: return vec3(0);\n"
        "    }\n"
        "}\n";

    return src + "\n" + switch_func;
}

// Create the shader program with a fragment shader.
static GLuint create_shader_program(
    const mi::base::Handle<const mi::neuraylib::ITarget_code>& target_code)
{
    const GLuint program = glCreateProgram();

    //vertex program
    if (program) {
        const std::string vs = read_text_file(get_executable_folder() + "/" + vertex_shader_filename);
        std::cout << vs << std::endl << std::endl;
        add_shader(GL_VERTEX_SHADER, vs, program);
    }

    //fragment program 1
    if (program) {
        std::stringstream sstr;
        sstr << "#version 330 core\n";
        sstr << "#define MAX_MATERIALS " << to_string(MAX_MATERIALS) << "\n";
        sstr << "#define MAX_TEXTURES " << to_string(MAX_TEXTURES) << "\n";
        sstr << read_text_file(get_executable_folder() + "/" + fragment_shader_filename);
        const std::string fp = sstr.str();
        std::cout << "Fragment program 1:\n" << fp << std::endl << std::endl;
        add_shader(GL_FRAGMENT_SHADER, fp, program);
    }

    //fragment program 2, MDL implementation
    if (program) {
        std::string code(target_code->get_code());
#ifdef REMAP_NOISE_FUNCTIONS
        code.append(read_text_file(get_executable_folder() + "/" + "noise_no_lut.glsl"));
#endif
        std::cout << "Fragment program 2:\n" << code << std::endl << std::endl;
        add_shader(GL_FRAGMENT_SHADER, code, program);
    }

    //fragment program 3, selection 
    if (program) {
        // Generate GLSL switch function for the generated functions
        const std::string glsl_switch_func = generate_glsl_switch_func(target_code);
#ifdef DUMP_GLSL
        std::cout << "Dumping GLSL code for the \"mdl_mat_subexpr\" switch function:\n\n"
            << glsl_switch_func << std::endl;
#endif
        std::cout << "Fragment program 3:\n" << glsl_switch_func << std::endl << std::endl;
        add_shader(GL_FRAGMENT_SHADER, glsl_switch_func, program);
    }

    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        dump_program_info(program, "Error linking the shader program: ");
        terminate();
    }

    glUseProgram(program);
    check_gl_success();

    return program;
}

// Create a quad filling the whole screen.
static GLuint create_quad(GLuint program, GLuint* vertex_buffer)
{
    const Vertex const vertices[6] = {
        { { -1.f, -1.f, 0.0f }, { 0.f, 0.f } },
        { {  1.f, -1.f, 0.0f }, { 1.f, 0.f } },
        { { -1.f,  1.f, 0.0f }, { 0.f, 1.f } },
        { {  1.f, -1.f, 0.0f }, { 1.f, 0.f } },
        { {  1.f,  1.f, 0.0f }, { 1.f, 1.f } },
        { { -1.f,  1.f, 0.0f }, { 0.f, 1.f } }
    };

    glGenBuffers(1, vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Get locations of vertex shader inputs
    const GLint pos_index = glGetAttribLocation(program, "Position");
    const GLint tex_coord_index = glGetAttribLocation(program, "TexCoord");
    check_success(pos_index >= 0 && tex_coord_index >= 0);

    glEnableVertexAttribArray(pos_index);
    glVertexAttribPointer(pos_index, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

    glEnableVertexAttribArray(tex_coord_index);
    glVertexAttribPointer(
        tex_coord_index, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(mi::Float32_3_struct)));

    check_gl_success();

    return vertex_array;
}

//------------------------------------------------------------------------------
//
// Material_opengl_context class
//
//------------------------------------------------------------------------------

// Helper class responsible for making textures and read-only data available to OpenGL
// by generating and managing a list of Material_data objects.
class Material_opengl_context
{
public:
    Material_opengl_context(GLuint program)
    : m_program(program)
    , m_next_storage_block_binding(0)
    {}

    // Free all acquired resources.
    ~Material_opengl_context();

    // Prepare the needed material data of the given target code.
    bool prepare_material_data(
        mi::base::Handle<mi::neuraylib::ITransaction>       transaction,
        mi::base::Handle<mi::neuraylib::IImage_api>         image_api,
        mi::base::Handle<const mi::neuraylib::ITarget_code> target_code);

    // Sets all collected material data in the OpenGL program.
    bool set_material_data();

private:
    // Sets the read-only data segments in the current OpenGL program object.
    void set_mdl_readonly_data(mi::base::Handle<const mi::neuraylib::ITarget_code> target_code);

    // Prepare the texture identified by the texture_index for use by the texture access functions
    // in the OpenGL program.
    bool prepare_texture(
        mi::base::Handle<mi::neuraylib::ITransaction>       transaction,
        mi::base::Handle<mi::neuraylib::IImage_api>         image_api,
        mi::base::Handle<const mi::neuraylib::ITarget_code> code_ptx,
        mi::Size                                            texture_index,
        GLuint                                              texture_array);

private:
    // The OpenGL program to prepare.
    GLuint m_program;

    GLuint m_next_storage_block_binding;

    std::vector<GLuint> m_texture_objects;

    std::vector<unsigned int> m_material_texture_starts;

    std::vector<GLuint> m_buffer_objects;
};

// Free all acquired resources.
Material_opengl_context::~Material_opengl_context()
{
    if (m_buffer_objects.size() > 0) {
        //glDeleteBuffers(GLsizei(m_buffer_objects.size()), &m_buffer_objects[0]);
    }

    if (m_texture_objects.size() > 0) {
        //glDeleteTextures(GLsizei(m_texture_objects.size()), &m_texture_objects[0]);
    }

    check_gl_success();
}

// Sets the read-only data segments in the current OpenGL program object.
void Material_opengl_context::set_mdl_readonly_data(
    mi::base::Handle<const mi::neuraylib::ITarget_code> target_code)
{
    mi::Size num_uniforms = target_code->get_ro_data_segment_count();
    if (num_uniforms == 0) return;

    if (use_ssbo()) {
        size_t cur_buffer_offs = m_buffer_objects.size();
        m_buffer_objects.insert(m_buffer_objects.end(), num_uniforms, 0);

        glGenBuffers(GLsizei(num_uniforms), &m_buffer_objects[cur_buffer_offs]);

        for (mi::Size i = 0; i < num_uniforms; ++i) {
            mi::Size segment_size = target_code->get_ro_data_segment_size(i);
            char const* segment_data = target_code->get_ro_data_segment_data(i);

#ifdef DUMP_GLSL
            std::cout << "Dump ro segment data " << i << " \""
                << target_code->get_ro_data_segment_name(i) << "\" (size = "
                << segment_size << "):\n" << std::hex;

            for (int j = 0; j < 16 && j < segment_size; ++j) {
                std::cout << "0x" << (unsigned int)(unsigned char)segment_data[j] << ", ";
            }
            std::cout << std::dec << std::endl;
#endif

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer_objects[cur_buffer_offs + i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(segment_size), segment_data, GL_STATIC_DRAW);

            GLuint block_index = glGetProgramResourceIndex(
                m_program, GL_SHADER_STORAGE_BLOCK, target_code->get_ro_data_segment_name(i));
            glShaderStorageBlockBinding(m_program, block_index, m_next_storage_block_binding);
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                m_next_storage_block_binding,
                m_buffer_objects[cur_buffer_offs + i]);

            ++m_next_storage_block_binding;

            check_gl_success();
        }
    }
    else {
        std::vector<char const*> uniform_names;
        for (mi::Size i = 0; i < num_uniforms; ++i) {
#ifdef DUMP_GLSL
            mi::Size segment_size = target_code->get_ro_data_segment_size(i);
            const char* segment_data = target_code->get_ro_data_segment_data(i);

            std::cout << "Dump ro segment data " << i << " \""
                << target_code->get_ro_data_segment_name(i) << "\" (size = "
                << segment_size << "):\n" << std::hex;

            for (int i = 0; i < 16 && i < segment_size; ++i) {
                std::cout << "0x" << (unsigned int)(unsigned char)segment_data[i] << ", ";
            }
            std::cout << std::dec << std::endl;
#endif

            uniform_names.push_back(target_code->get_ro_data_segment_name(i));
        }

        std::vector<GLuint> uniform_indices(num_uniforms, 0);
        glGetUniformIndices(m_program, GLsizei(num_uniforms), &uniform_names[0], &uniform_indices[0]);

        for (mi::Size i = 0; i < num_uniforms; ++i) {
            // uniforms may have been removed, if they were not used
            if (uniform_indices[i] == GL_INVALID_INDEX)
                continue;

            GLint uniform_type = 0;
            GLuint index = GLuint(uniform_indices[i]);
            glGetActiveUniformsiv(m_program, 1, &index, GL_UNIFORM_TYPE, &uniform_type);

#ifdef DUMP_GLSL
            std::cout << "Uniform type of " << uniform_names[i]
                << ": 0x" << std::hex << uniform_type << std::dec << std::endl;
#endif

            mi::Size segment_size = target_code->get_ro_data_segment_size(i);
            const char* segment_data = target_code->get_ro_data_segment_data(i);

            GLint uniform_location = glGetUniformLocation(m_program, uniform_names[i]);

            switch (uniform_type) {

                // For bool, the data has to be converted to int, first
#define CASE_TYPE_BOOL(type, func, num)                            \
    case type: {                                                   \
        GLint *buf = new GLint[segment_size];                      \
        for (mi::Size j = 0; j < segment_size; ++j)               \
            buf[j] = GLint(segment_data[j]);                       \
        func(uniform_location, GLsizei(segment_size / num), buf);  \
        delete[] buf;                                              \
        break;                                                     \
    }

                CASE_TYPE_BOOL(GL_BOOL, glUniform1iv, 1)
                    CASE_TYPE_BOOL(GL_BOOL_VEC2, glUniform2iv, 2)
                    CASE_TYPE_BOOL(GL_BOOL_VEC3, glUniform3iv, 3)
                    CASE_TYPE_BOOL(GL_BOOL_VEC4, glUniform4iv, 4)

#define CASE_TYPE(type, func, num, elemtype)                                      \
    case type:                                                                    \
        func(uniform_location, GLsizei(segment_size/(num * sizeof(elemtype))), \
            (const elemtype*)segment_data);                                       \
        break

                    CASE_TYPE(GL_INT, glUniform1iv, 1, GLint);
                CASE_TYPE(GL_INT_VEC2, glUniform2iv, 2, GLint);
                CASE_TYPE(GL_INT_VEC3, glUniform3iv, 3, GLint);
                CASE_TYPE(GL_INT_VEC4, glUniform4iv, 4, GLint);
                CASE_TYPE(GL_FLOAT, glUniform1fv, 1, GLfloat);
                CASE_TYPE(GL_FLOAT_VEC2, glUniform2fv, 2, GLfloat);
                CASE_TYPE(GL_FLOAT_VEC3, glUniform3fv, 3, GLfloat);
                CASE_TYPE(GL_FLOAT_VEC4, glUniform4fv, 4, GLfloat);
                CASE_TYPE(GL_DOUBLE, glUniform1dv, 1, GLdouble);
                CASE_TYPE(GL_DOUBLE_VEC2, glUniform2dv, 2, GLdouble);
                CASE_TYPE(GL_DOUBLE_VEC3, glUniform3dv, 3, GLdouble);
                CASE_TYPE(GL_DOUBLE_VEC4, glUniform4dv, 4, GLdouble);

#define CASE_TYPE_MAT(type, func, num, elemtype)                                  \
    case type:                                                                    \
        func(uniform_location, GLsizei(segment_size/(num * sizeof(elemtype))),  \
            false, (const elemtype*)segment_data);                                \
        break

                CASE_TYPE_MAT(GL_FLOAT_MAT2_ARB, glUniformMatrix2fv, 4, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT2x3, glUniformMatrix2x3fv, 6, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT3x2, glUniformMatrix3x2fv, 6, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT2x4, glUniformMatrix2x4fv, 8, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT4x2, glUniformMatrix4x2fv, 8, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT3_ARB, glUniformMatrix3fv, 9, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT3x4, glUniformMatrix3x4fv, 12, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT4x3, glUniformMatrix4x3fv, 12, GLfloat);
                CASE_TYPE_MAT(GL_FLOAT_MAT4_ARB, glUniformMatrix4fv, 16, GLfloat);
                CASE_TYPE_MAT(GL_DOUBLE_MAT2, glUniformMatrix2dv, 4, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT2x3, glUniformMatrix2x3dv, 6, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT3x2, glUniformMatrix3x2dv, 6, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT2x4, glUniformMatrix2x4dv, 8, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT4x2, glUniformMatrix4x2dv, 8, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT3, glUniformMatrix3dv, 9, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT3x4, glUniformMatrix3x4dv, 12, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT4x3, glUniformMatrix4x3dv, 12, GLdouble);
                CASE_TYPE_MAT(GL_DOUBLE_MAT4, glUniformMatrix4dv, 16, GLdouble);

            default:
                std::cerr << "Unsupported uniform type: 0x"
                    << std::hex << uniform_type << std::dec << std::endl;
                terminate();
                break;
            }

            check_gl_success();
        }
    }
}

// Prepare the texture identified by the texture_index for use by the texture access functions
// on the GPU.
bool Material_opengl_context::prepare_texture(
    mi::base::Handle<mi::neuraylib::ITransaction>       transaction,
    mi::base::Handle<mi::neuraylib::IImage_api>         image_api,
    mi::base::Handle<const mi::neuraylib::ITarget_code> code,
    mi::Size                                            texture_index,
    GLuint                                              texture_obj)
{
    // Get access to the texture data by the texture database name from the target code.
    mi::base::Handle<const mi::neuraylib::ITexture> texture(
        transaction->access<mi::neuraylib::ITexture>(code->get_texture(texture_index)));
    mi::base::Handle<const mi::neuraylib::IImage> image(
        transaction->access<mi::neuraylib::IImage>(texture->get_image()));
    mi::base::Handle<const mi::neuraylib::ICanvas> canvas(image->get_canvas());
    mi::Uint32 tex_width = canvas->get_resolution_x();
    mi::Uint32 tex_height = canvas->get_resolution_y();
    mi::Uint32 tex_layers = canvas->get_layers_size();
    char const *image_type = image->get_type();

    if (canvas->get_tiles_size_x() != 1 || canvas->get_tiles_size_y() != 1) {
        std::cerr << "The example does not support tiled images!" << std::endl;
        return false;
    }

    if (tex_layers != 1) {
        std::cerr << "The example and the GLSL backend don't support layered images!" << std::endl;
        return false;
    }

    // For simplicity, the texture access functions are only implemented for float4 and gamma
    // is pre-applied here (all images are converted to linear space).

    // Convert to linear color space if necessary
    if (texture->get_effective_gamma() != 1.0f) {
        // Copy/convert to float4 canvas and adjust gamma from "effective gamma" to 1.
        mi::base::Handle<mi::neuraylib::ICanvas> gamma_canvas(
            image_api->convert(canvas.get(), "Color"));
        gamma_canvas->set_gamma(texture->get_effective_gamma());
        image_api->adjust_gamma(gamma_canvas.get(), 1.0f);
        canvas = gamma_canvas;
    } else if (strcmp(image_type, "Color") != 0 && strcmp(image_type, "Float32<4>") != 0) {
        // Convert to expected format
        canvas = image_api->convert(canvas.get(), "Color");
    }

    // This example supports only 2D textures
    mi::neuraylib::ITarget_code::Texture_shape texture_shape =
        code->get_texture_shape(texture_index);
    if (texture_shape == mi::neuraylib::ITarget_code::Texture_shape_2d) {
        mi::base::Handle<const mi::neuraylib::ITile> tile(canvas->get_tile(0, 0));
        mi::Float32 const *data = static_cast<mi::Float32 const *>(tile->get_data());

        glBindTexture(GL_TEXTURE_2D, texture_obj);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0,  GL_RGBA, GL_FLOAT, data);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    check_gl_success();

    return true;
}

// Prepare the needed material data of the given target code.
bool Material_opengl_context::prepare_material_data(
    mi::base::Handle<mi::neuraylib::ITransaction>       transaction,
    mi::base::Handle<mi::neuraylib::IImage_api>         image_api,
    mi::base::Handle<const mi::neuraylib::ITarget_code> target_code)
{
    // Handle the read-only data segments if necessary
    set_mdl_readonly_data(target_code);

    // Handle the textures if there are more than just the invalid texture
    const size_t cur_tex_offs = m_texture_objects.size();
    m_material_texture_starts.push_back(GLuint(cur_tex_offs));

    const mi::Size num_textures = target_code->get_texture_count();
    if (num_textures > 1) {
        m_texture_objects.insert(m_texture_objects.end(), num_textures - 1, 0);

        glGenTextures(GLsizei(num_textures - 1), &m_texture_objects[cur_tex_offs]);

        // Loop over all textures skipping the first texture,
        // which is always the MDL invalid texture
        for (mi::Size i = 1; i < num_textures; ++i) {
            if (!prepare_texture(
                    transaction, image_api, target_code,
                    i, m_texture_objects[cur_tex_offs + i - 1]))
                return false;
        }
    }

    return true;
}

// Sets all collected material data in the OpenGL program.
bool Material_opengl_context::set_material_data()
{
    GLsizei total_textures = GLsizei(m_texture_objects.size());

    if(total_textures > MAX_TEXTURES)
    {
        fprintf( stderr, "Number of required textures (%d) is not supported (max: %d)\n",
        total_textures, MAX_TEXTURES);
        return false;
    }

    if (use_ssbo()) {
        if (glfwExtensionSupported("GL_ARB_bindless_texture"))
        {
            if (total_textures > 0) {
                std::vector<GLuint64> texture_handles;
                texture_handles.resize(total_textures);
                for (GLsizei i = 0; i < total_textures; ++i) {
                    texture_handles[i] = glGetTextureHandleARB(m_texture_objects[i]);
                    glMakeTextureHandleResidentARB(texture_handles[i]);
                }

                glUniformHandleui64vARB(
                    glGetUniformLocation(m_program, "material_texture_samplers_2d"),
                    total_textures,
                    &texture_handles[0]);

                glUniform1uiv(
                    glGetUniformLocation(m_program, "material_texture_starts"),
                    GLsizei(m_material_texture_starts.size()),
                    &m_material_texture_starts[0]);
            }
        }
        else if (glfwExtensionSupported("GL_NV_bindless_texture"))
        {
            if (total_textures > 0) {
                std::vector<GLuint64> texture_handles;
                texture_handles.resize(total_textures);
                for (GLsizei i = 0; i < total_textures; ++i) {
                    texture_handles[i] = glGetTextureHandleNV(m_texture_objects[i]);
                    glMakeTextureHandleResidentNV(texture_handles[i]);
                }

                glUniformHandleui64vARB(
                    glGetUniformLocation(m_program, "material_texture_samplers_2d"),
                    total_textures,
                    &texture_handles[0]);

                glUniform1uiv(
                    glGetUniformLocation(m_program, "material_texture_starts"),
                    GLsizei(m_material_texture_starts.size()),
                    &m_material_texture_starts[0]);
            }
        }
        else
        {
            fprintf(stderr, "Sample requires Bindless Textures, "
                "that are not supported by the current system.\n");
            return false;
        }
    }

    // Check for any errors. If you get an error, check whether MAX_TEXTURES and MAX_MATERIALS
    // in example_execution_glsl.frag still fit to your needs.
    return glGetError() == GL_NO_ERROR;
}


//------------------------------------------------------------------------------
//
// MDL material compilation code
//
//------------------------------------------------------------------------------

class Material_compiler {

public: 
    // Constructor.
    Material_compiler()
        : m_mdl_compiler(nullptr)
        , m_be_glsl(nullptr)
        , m_transaction(nullptr)
        , m_context(nullptr)
        , m_link_unit()
    {}

    void init(
        mi::neuraylib::IMdl_compiler* mdl_compiler,
        mi::neuraylib::IMdl_factory* mdl_factory,
        mi::neuraylib::ITransaction* transaction);

    // Generates GLSL target code for a subexpression of a given material.
    // path is the path of the sub-expression.
    // fname is the function name in the generated code.
    bool add_material_subexpr(
        const std::string& material_name,
        const char* path,
        const char* fname);

    bool add_material(const std::string& material_name);

    // Generates GLSL target code for a subexpression of a given compiled material.
    mi::base::Handle<const mi::neuraylib::ITarget_code> generate_glsl();

private:
    // Helper function to extract the module name from a fully-qualified material name.
    static std::string get_module_name(const std::string& material_name);

    // Helper function to extract the material name from a fully-qualified material name.
    static std::string get_material_name(const std::string& material_name);

    // Creates an instance of the given material.
    mi::neuraylib::IMaterial_instance* create_material_instance(
        const std::string& material_name);

    // Compiles the given material instance in the given compilation modes.
    mi::neuraylib::ICompiled_material* compile_material_instance(
        mi::neuraylib::IMaterial_instance* material_instance,
        bool class_compilation);

private:
    mi::base::Handle<mi::neuraylib::IMdl_compiler> m_mdl_compiler;
    mi::base::Handle<mi::neuraylib::IMdl_backend>  m_be_glsl;
    mi::base::Handle<mi::neuraylib::ITransaction>  m_transaction;
    mi::base::Handle<mi::neuraylib::IMdl_execution_context>
                                                   m_context;
    mi::base::Handle<mi::neuraylib::ILink_unit> m_link_unit;
};

// Helper function to extract the module name from a fully-qualified material name.
std::string Material_compiler::get_module_name(const std::string& material_name)
{
    std::size_t p = material_name.rfind("::");
    return material_name.substr(0, p);
}

// Helper function to extract the material name from a fully-qualified material name.
std::string Material_compiler::get_material_name(const std::string& material_name)
{
    std::size_t p = material_name.rfind("::");
    if (p == std::string::npos)
        return material_name;
    return material_name.substr(p + 2, material_name.size() - p);
}

// Creates an instance of the given material.
mi::neuraylib::IMaterial_instance* Material_compiler::create_material_instance(
    const std::string& material_name)
{
    // Load mdl module.
    std::string module_name = get_module_name(material_name);
    check_success(m_mdl_compiler->load_module(
        m_transaction.get(), module_name.c_str(), m_context.get()) >= 0);
    print_messages(m_context.get());

    // Create a material instance from the material definition 
    // with the default arguments.
    std::string material_db_name = "mdl" + material_name;
    mi::base::Handle<const mi::neuraylib::IMaterial_definition> material_definition(
        m_transaction->access<mi::neuraylib::IMaterial_definition>(
            material_db_name.c_str()));
    check_success(material_definition);

    mi::Sint32 result;
    mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
        material_definition->create_material_instance(0, &result));
    check_success(result == 0);

    material_instance->retain();
    return material_instance.get();
}

// Compiles the given material instance in the given compilation modes.
mi::neuraylib::ICompiled_material *Material_compiler::compile_material_instance(
    mi::neuraylib::IMaterial_instance* material_instance,
    bool class_compilation)
{
    mi::Sint32 result = 0;
    mi::Uint32 flags = class_compilation
        ? mi::neuraylib::IMaterial_instance::CLASS_COMPILATION
        : mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS;
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        material_instance->create_compiled_material(flags, m_context.get()));
    check_success(print_messages(m_context.get()));

    compiled_material->retain();
    return compiled_material.get();
}

// Generates GLSL target code for a subexpression of a given compiled material.
mi::base::Handle<const mi::neuraylib::ITarget_code> Material_compiler::generate_glsl()
{
    mi::Sint32 result = -1;
    mi::base::Handle<const mi::neuraylib::ITarget_code> code_glsl(
        m_be_glsl->translate_link_unit(m_link_unit.get(), m_context.get()));
    check_success(print_messages(m_context.get()));
    check_success(code_glsl);

#ifdef DUMP_GLSL
    std::cout << "Dumping GLSL code:\n\n" << code_glsl->get_code() << std::endl;
#endif

    return code_glsl;
}

// Generates GLSL target code for a subexpression of a given material.
// path is the path of the sub-expression.
// fname is the function name in the generated code.
bool Material_compiler::add_material_subexpr(
    const std::string& material_name,
    const char* path,
    const char* fname)
{
    // Load the given module and create a material instance
    mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
        create_material_instance(material_name.c_str()));

    // Compile the material instance in instance compilation mode
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        compile_material_instance(material_instance.get(), /*class_compilation=*/false));

    m_link_unit->add_material_expression(compiled_material.get(), path, fname, m_context.get());

    auto context = m_context.get();
    return print_messages(context);
}


bool Material_compiler::add_material(const std::string& material_name)
{
    // Load the given module and create a material instance
    mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
        create_material_instance(material_name.c_str()));

    // Compile the material instance in instance compilation mode
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        compile_material_instance(material_instance.get(), /*class_compilation=*/false));

    m_link_unit->add_material(compiled_material.get(), nullptr, 0, m_context.get());

    auto context = m_context.get();
    return print_messages(context);
}


// Constructor.
void Material_compiler::init(
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_factory* mdl_factory,
    mi::neuraylib::ITransaction* transaction)
{
    m_mdl_compiler = (mi::base::make_handle_dup(mdl_compiler));
    m_be_glsl = (mdl_compiler->get_backend(mi::neuraylib::IMdl_compiler::MB_GLSL));
    m_transaction = (mi::base::make_handle_dup(transaction));
    m_context = (mdl_factory->create_execution_context());

    check_success(m_be_glsl->set_option("num_texture_spaces", "1") == 0);

    if (use_ssbo()) {
        // SSBO requires GLSL 4.30
        check_success(m_be_glsl->set_option("glsl_version", "430") == 0);
    }
    else {
        check_success(m_be_glsl->set_option("glsl_version", "330") == 0);
    }

    // Specify the implementation modes for some state functions.
    // Note that "geometry_normal", "normal" and "position" default to "field" mode.
    check_success(m_be_glsl->set_option("glsl_state_animation_time_mode", "field") == 0);
    check_success(m_be_glsl->set_option("glsl_state_position_mode", "func") == 0);
    check_success(m_be_glsl->set_option("glsl_state_texture_coordinate_mode", "arg") == 0);
    check_success(m_be_glsl->set_option("glsl_state_texture_tangent_u_mode", "field") == 0);
    check_success(m_be_glsl->set_option("glsl_state_texture_tangent_v_mode", "field") == 0);

    if (use_ssbo()) {
        check_success(m_be_glsl->set_option("glsl_max_const_data", "0") == 0);
        check_success(m_be_glsl->set_option("glsl_place_uniforms_into_ssbo", "on") == 0);
    }
    else {
        check_success(m_be_glsl->set_option("glsl_max_const_data", "1024") == 0);
        check_success(m_be_glsl->set_option("glsl_place_uniforms_into_ssbo", "off") == 0);
    }

#ifdef REMAP_NOISE_FUNCTIONS
    // remap noise functions that access the constant tables
    check_success(m_be_glsl->set_option(
        "glsl_remap_functions",
        "_ZN4base12perlin_noiseEu6float4=noise_float4"
        ",_ZN4base12worley_noiseEu6float3fi=noise_worley"
        ",_ZN4base8mi_noiseEu6float3=noise_mi_float3"
        ",_ZN4base8mi_noiseEu4int3=noise_mi_int3") == 0);
#endif

    // After we set the options, we can create the link unit
    m_link_unit = mi::base::make_handle(m_be_glsl->create_link_unit(transaction, m_context.get()));
}


//------------------------------------------------------------------------------
//
// Application logic
//
//------------------------------------------------------------------------------

// Initializes OpenGL, creates the shader program and the scene and executes the animation loop.
static void show_and_animate_scene(GLFWwindow *window, GLuint program, Window_context &window_context)
{    
    // Create scene data
    GLuint quad_vertex_buffer = 0;
    const GLuint quad_vao = create_quad(program, &quad_vertex_buffer);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Set uniform frame parameters
        const GLint material_pattern_index = glGetUniformLocation(program, "material_pattern");
        const GLint animation_time_index = glGetUniformLocation(program, "animation_time");
        glUniform1ui(material_pattern_index, window_context.material_pattern);
        glUniform1f(animation_time_index, glfwGetTime());

        // Render the scene
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);

        // Poll for events and process them
        glfwPollEvents();
    }

    // Cleanup OpenGL
    glDeleteVertexArrays(1, &quad_vao);
    glDeleteBuffers(1, &quad_vertex_buffer);
    check_gl_success();
}


void usage(char const *prog_name)
{
    std::cout
        << "Usage: " << prog_name << " [options] [<material_pattern>]\n"
        << "Options:\n"
        << "  --nowin             don't show interactive display\n"
        << "  --res <x> <y>       resolution (default: 1024x768)\n"
        << "  -o <outputfile>     image file to write result in nowin mode (default: output.png)\n"
        << "  <material_pattern>  a number from 1 to 7 choosing which material combination to use"
        << std::endl;
    keep_console_open();
    exit(EXIT_FAILURE);
}


static void parse(int argc, char **argv, Options &options)
{
    for (int i = 1; i < argc; ++i) {
        char const *opt = argv[i];
        if (opt[0] == '-') {
            if (strcmp(opt, "--nowin") == 0)
                options.no_window = true;
            else if (strcmp(opt, "-o") == 0) {
                if (i < argc - 1)
                    options.outputfile = argv[++i];
                else
                    usage(argv[0]);
            }
            else if (strcmp(opt, "--res") == 0) {
                if (i < argc - 2) {
                    options.res_x = std::max(atoi(argv[++i]), 1);
                    options.res_y = std::max(atoi(argv[++i]), 1);
                }
                else
                    usage(argv[0]);
            }
            else
                usage(argv[0]);
        }
        else {
            options.material_pattern = unsigned(atoi(opt));
            if (options.material_pattern < 1 || options.material_pattern > 7) {
                std::cerr << "Invalid material_pattern parameter." << std::endl;
                usage(argv[0]);
            }
        }
    }
}

static GLuint setup_material(mi::base::Handle<mi::neuraylib::INeuray> neuray)
{
    // Create a transaction
    mi::base::Handle<mi::neuraylib::ITransaction> transaction; {
        mi::base::Handle<mi::neuraylib::IDatabase> database(neuray->get_api_component<mi::neuraylib::IDatabase>());
        mi::base::Handle<mi::neuraylib::IScope> scope(database->get_global_scope());
        transaction = mi::base::Handle<mi::neuraylib::ITransaction>(scope->create_transaction());
    }

    // Generate the GLSL code for the link unit.
    mi::base::Handle<const mi::neuraylib::ITarget_code> target_code; {
        // Access the MDL SDK compiler component
        Material_compiler mc;
        {
            // Access MDL factory
            mi::base::Handle<mi::neuraylib::IMdl_factory> mdl_factory(
                neuray->get_api_component<mi::neuraylib::IMdl_factory>());

            // Create a material compiler 
            mi::base::Handle<mi::neuraylib::IMdl_compiler> mdl_compiler(
                neuray->get_api_component<mi::neuraylib::IMdl_compiler>());

            mc.init(mdl_compiler.get(), mdl_factory.get(), transaction.get());
/*
            // Add material sub-expressions of different materials to the link unit.
#if defined(USE_SSBO) || defined(REMAP_NOISE_FUNCTIONS)
            // this uses a lot of constant data
            mc.add_material_subexpr(
                "::nvidia::sdk_examples::tutorials::example_execution1",
                "surface.scattering.tint", "tint");
#endif

            mc.add_material_subexpr(
                "::nvidia::sdk_examples::tutorials::example_execution2",
                "surface.scattering.tint", "tint_2");

#if defined(USE_SSBO) || defined(REMAP_NOISE_FUNCTIONS)
            // this uses a lot of constant data
            mc.add_material_subexpr(
                "::nvidia::sdk_examples::tutorials::example_execution3",
                "surface.scattering.tint", "tint_3");
#endif
*/

            mc.add_material("::nvidia::sdk_examples::gun_metal::gun_metal");
        }

        target_code = mc.generate_glsl();
    }

    // Create shader program
    const GLuint program = create_shader_program(target_code);
    if (program) {
        // Acquire image API needed to prepare the textures
        mi::base::Handle<mi::neuraylib::IImage_api> image_api(neuray->get_api_component<mi::neuraylib::IImage_api>());

        // Prepare the needed material data of all target codes for the fragment shader
        Material_opengl_context material_opengl_context(program);
        check_success(material_opengl_context.prepare_material_data(transaction, image_api, target_code));
        check_success(material_opengl_context.set_material_data());
    }

    transaction->commit();

    return program;
}

//------------------------------------------------------------------------------
//
// Main function
//
//------------------------------------------------------------------------------
static mi::base::Handle<mi::neuraylib::INeuray> neuray(nullptr);

static void mdlsdk_init(void)
{
    // Access the MDL SDK
    if (neuray != nullptr) return;

    //mi::base::Handle<mi::neuraylib::INeuray> neuray(load_and_get_ineuray());
    neuray = load_and_get_ineuray();
    check_success(neuray.is_valid_interface());
    // Configure the MDL SDK
    configure(neuray.get());

    // Start the MDL SDK
    mi::Sint32 result = neuray->start();
    check_start_success(result);

}

static void mdlsdk_stop(void)
{
    // Shut down the MDL SDK
    check_success(neuray->shutdown() == 0);
    neuray = nullptr;

    // Unload the MDL SDK
    check_success(unload());
}

}//namespace



int main(int argc, char* argv[])
{
    mdlsdk_init();

    // Init OpenGL window
    GLFWwindow *window = nullptr; {
        // Parse command line options
        Options options;
        parse(argc, argv, options);
        window = init_opengl(options);
        const GLuint program = setup_material(neuray);
        show_and_animate_scene(window, program, window_context);
        glDeleteProgram(program);
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    mdlsdk_stop();

    keep_console_open();

    return EXIT_SUCCESS;
}


