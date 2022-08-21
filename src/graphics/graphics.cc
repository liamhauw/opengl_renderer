#include "graphics/graphics.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "GL/glew.h"
#include "configure/root_directory.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "stb/stb_image.h"

namespace graphics {
std::string root_directory{root_directory_in};

Camera camera{glm::vec3{0.0f, 0.0f, 10.0f}};

float last_cursor_xpos;
float last_cursor_ypos;
bool first_cursor{true};

float current_time;
float last_time{0.0f};
float delta_time{0.0f};

bool enable_camera_move{false};

unsigned sphere_vao{0};
unsigned sphere_index_count;
unsigned cube_vao{0};
unsigned quad_vao{0};

unsigned LoadTexture(const std::string &path) {
  unsigned texture_id;
  glGenTextures(1, &texture_id);

  int width, height, component_count;
  unsigned char *data =
      stbi_load(path.c_str(), &width, &height, &component_count, 0);
  if (data) {
    GLenum format;
    if (component_count == 1) {
      format = GL_RED;
    } else if (component_count == 3) {
      format = GL_RGB;
    } else if (component_count == 4) {
      format = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    throw "fail to load texture at " + path;
  }
  stbi_image_free(data);
  return texture_id;
}

std::vector<std::vector<unsigned>> LoadPbrTexture(const char *material[],
                                                  unsigned count) {
  std::string path;
  std::vector<std::vector<unsigned>> pbr_texture;
  for (unsigned u = 0; u < count; ++u) {
    std::vector<unsigned> texture;
    path = std::string{root_directory} + "/resource/texture/pbr/" + material[u];
    texture.push_back(LoadTexture(path + "/normal.png"));
    texture.push_back(LoadTexture(path + "/albedo.png"));
    texture.push_back(LoadTexture(path + "/metallic.png"));
    texture.push_back(LoadTexture(path + "/roughness.png"));
    texture.push_back(LoadTexture(path + "/ao.png"));
    pbr_texture.push_back(texture);
  }
  return pbr_texture;
}

void ErrorCallback(int error, const char *description) {
  throw std::string{description};
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
    enable_camera_move = !enable_camera_move;
    if (enable_camera_move) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      first_cursor = true;
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

void CursorPosCallback(GLFWwindow *window, double xposition, double yposition) {
  if (enable_camera_move) {
    if (first_cursor) {
      last_cursor_xpos = xposition;
      last_cursor_ypos = yposition;
      first_cursor = false;
    }

    float xoffset = xposition - last_cursor_xpos;
    float yoffset = last_cursor_ypos - yposition;

    last_cursor_xpos = xposition;
    last_cursor_ypos = yposition;

    camera.ProcessCursorPos(xoffset, yoffset);
  }
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  if (enable_camera_move) {
    camera.ProcessScroll(yoffset);
  }
}

void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow *window) {
  if (enable_camera_move) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      camera.ProcessKey(kForward, delta_time);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      camera.ProcessKey(kBackward, delta_time);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      camera.ProcessKey(kLeft, delta_time);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      camera.ProcessKey(kRight, delta_time);
    }
  }
}

void RenderSphere() {
  if (sphere_vao == 0) {
    glGenVertexArrays(1, &sphere_vao);

    unsigned vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texture_coordinate;
    std::vector<unsigned> index;

    const unsigned kXsegmentCount = 64;
    const unsigned kYsegmentCount = 64;
    const float kPi = 3.14159265359f;
    for (unsigned x = 0; x <= kXsegmentCount; ++x) {
      for (unsigned y = 0; y <= kYsegmentCount; ++y) {
        float xsegment =
            static_cast<float>(x) / static_cast<float>(kXsegmentCount);
        float ysegment =
            static_cast<float>(y) / static_cast<float>(kYsegmentCount);
        float xposition =
            2.0f * std::cos(xsegment * 2.0f * kPi) * std::sin(ysegment * kPi);
        float yposition = 2.0f * std::cos(ysegment * kPi);
        float zposition =
            2.0f * std::sin(xsegment * 2.0f * kPi) * std::sin(ysegment * kPi);

        position.push_back(glm::vec3(xposition, yposition, zposition));
        normal.push_back(glm::vec3(xposition, yposition, zposition));
        texture_coordinate.push_back(glm::vec2(xsegment, ysegment));
      }
    }

    bool odd_row = false;
    for (unsigned y = 0; y < kYsegmentCount; ++y) {
      if (!odd_row) {
        for (unsigned x = 0; x <= kXsegmentCount; ++x) {
          index.push_back(y * (kXsegmentCount + 1) + x);
          index.push_back((y + 1) * (kXsegmentCount + 1) + x);
        }
      } else {
        for (int x = kXsegmentCount; x >= 0; --x) {
          index.push_back((y + 1) * (kXsegmentCount + 1) + x);
          index.push_back(y * (kXsegmentCount + 1) + x);
        }
      }
      odd_row = !odd_row;
    }
    sphere_index_count = index.size();

    std::vector<float> data;
    for (unsigned i = 0; i < position.size(); ++i) {
      data.push_back(position[i].x);
      data.push_back(position[i].y);
      data.push_back(position[i].z);
      if (normal.size() > 0) {
        data.push_back(normal[i].x);
        data.push_back(normal[i].y);
        data.push_back(normal[i].z);
      }
      if (texture_coordinate.size() > 0) {
        data.push_back(texture_coordinate[i].x);
        data.push_back(texture_coordinate[i].y);
      }
    }
    glBindVertexArray(sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0],
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(unsigned int),
                 &index[0], GL_STATIC_DRAW);
    unsigned stride = (3 + 2 + 3) * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(6 * sizeof(float)));
  }

  glBindVertexArray(sphere_vao);
  glDrawElements(GL_TRIANGLE_STRIP, sphere_index_count, GL_UNSIGNED_INT, 0);
}

void RenderCube() {
  if (cube_vao == 0) {
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
        1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
        -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,   // top-left
        // front face
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
        -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // top-left
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
        // left face
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
        -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
        -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
                                                             // right face
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // top-left
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // bottom-right
        1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,     // top-right
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // bottom-right
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // top-left
        1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
        1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,   // top-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
        -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
        // top face
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
        -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f    // bottom-left
    };
    unsigned vbo{0};
    glGenVertexArrays(1, &cube_vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(cube_vao);
    unsigned stride = (3 + 2 + 3) * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(6 * sizeof(float)));
  }

  glBindVertexArray(cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void RenderQuad() {
  if (quad_vao == 0) {
    unsigned int quad_vbo;
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
  }
  glBindVertexArray(quad_vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void GlCheckError(const char *file, int line) {
  GLenum error_flag;
  if ((error_flag = glGetError()) != GL_NO_ERROR) {
    std::string error;
    switch (error_flag) {
      case GL_INVALID_ENUM:
        error = "GL_INVALID_ENUM";
        break;
      case GL_INVALID_VALUE:
        error = "GL_INVALID_VALUE";
        break;
      case GL_INVALID_OPERATION:
        error = "GL_INVALID_OPERATION";
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        error = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
      case GL_OUT_OF_MEMORY:
        error = "GL_OUT_OF_MEMORY";
        break;
      case GL_STACK_UNDERFLOW:
        error = "GL_STACK_UNDERFLOW";
        break;
      case GL_STACK_OVERFLOW:
        error = "GL_STACK_OVERFLOW";
        break;
    }
    throw error + std::string{" occur in "} + file +
        std::string{" befor line "} + std::to_string(line);
  }
}

Camera::Camera(glm::vec3 position, glm::vec3 world_up, float yaw, float pitch)
    : position_{position},
      front_{glm::vec3{0.0f, 0.0f, -1.0f}},
      world_up_{world_up},
      yaw_{yaw},
      pitch_{pitch},
      speed_{2.5f},
      sensitivity_{0.1f},
      yfov_{45.0f} {
  UpdateCamera();
}

glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::ProcessKey(CameraDirection direction, float delta_time) {
  float velocity = speed_ * delta_time;
  if (direction == kForward) {
    position_ += front_ * velocity;
  }
  if (direction == kBackward) {
    position_ -= front_ * velocity;
  }
  if (direction == kLeft) {
    position_ -= right_ * velocity;
  }
  if (direction == kRight) {
    position_ += right_ * velocity;
  }
}

void Camera::ProcessCursorPos(float xoffset, float yoffset,
                              bool constrain_pitch) {
  xoffset *= sensitivity_;
  yoffset *= sensitivity_;
  yaw_ += xoffset;
  pitch_ += yoffset;
  if (constrain_pitch) {
    if (pitch_ > 89.0f) {
      pitch_ = 89.0f;
    }
    if (pitch_ < -89.0f) {
      pitch_ = -89.0f;
    }
  }
  UpdateCamera();
}

void Camera::ProcessScroll(float yoffset) {
  yfov_ -= yoffset;
  if (yfov_ < 1.0f) {
    yfov_ = 1.0f;
  }
  if (yfov_ > 45.0f) {
    yfov_ = 45.0f;
  }
}

void Camera::UpdateCamera() {
  glm::vec3 front;
  front.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
  front.y = std::sin(glm::radians(pitch_));
  front.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
  front_ = glm::normalize(front);
  right_ = glm::normalize(glm::cross(front_, world_up_));
  up_ = glm::normalize(glm::cross(right_, front_));
}

Shader::Shader(const std::string &vertex_shader,
               const std::string &fragment_shader,
               const std::string &geometry_shader) {
  std::string vs_string;
  std::string fs_string;
  std::string gs_string;

  std::ifstream vs_file{vertex_shader};
  if (!vs_file) {
    throw std::string{"fail to read "} + vertex_shader;
  }
  std::ifstream fs_file{fragment_shader};
  if (!fs_file) {
    throw std::string{"fail to read "} + fragment_shader;
  }
  std::stringstream vs_sstream;
  std::stringstream fs_sstream;
  vs_sstream << vs_file.rdbuf();
  fs_sstream << fs_file.rdbuf();
  vs_file.close();
  fs_file.close();
  vs_string = vs_sstream.str();
  fs_string = fs_sstream.str();
  if (geometry_shader != std::string{}) {
    std::ifstream gs_file{geometry_shader};
    if (!gs_file) {
      throw std::string{"fail to read "} + geometry_shader;
    }
    std::stringstream gs_sstream;
    gs_sstream << gs_file.rdbuf();
    gs_file.close();
    gs_string = gs_sstream.str();
  }

  const char *vs_source{vs_string.c_str()};
  const char *fs_source{fs_string.c_str()};
  unsigned int vs;
  unsigned int fs;
  vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_source, nullptr);
  glCompileShader(vs);
  CheckError(vs, "vertex shaderz");
  fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_source, nullptr);
  glCompileShader(fs);
  CheckError(fs, "fragment shader");
  unsigned int gs;
  if (geometry_shader != std::string{}) {
    const char *gs_src{gs_string.c_str()};
    gs = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(gs, 1, &gs_src, nullptr);
    glCompileShader(gs);
    CheckError(gs, "geometry shader");
  }

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  if (geometry_shader != std::string{}) {
    glAttachShader(program, gs);
  }
  glLinkProgram(program);
  CheckError(program, "program");

  glDeleteShader(vs);
  glDeleteShader(fs);
  if (geometry_shader != std::string{}) {
    glDeleteShader(gs);
  }
}

void Shader::UseProgram() { glUseProgram(program); }

void Shader::SetBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(program, name.c_str()),
              static_cast<int>(value));
}
void Shader::SetInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}
void Shader::SetFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}
void Shader::SetVec2(const std::string &name, const glm::vec2 &value) const {
  glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void Shader::SetVec2(const std::string &name, float x, float y) const {
  glUniform2f(glGetUniformLocation(program, name.c_str()), x, y);
}
void Shader::SetVec3(const std::string &name, const glm::vec3 &value) const {
  glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void Shader::SetVec3(const std::string &name, float x, float y, float z) const {
  glUniform3f(glGetUniformLocation(program, name.c_str()), x, y, z);
}
void Shader::SetVec4(const std::string &name, const glm::vec4 &value) const {
  glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void Shader::SetVec4(const std::string &name, float x, float y, float z,
                     float w) {
  glUniform4f(glGetUniformLocation(program, name.c_str()), x, y, z, w);
}
void Shader::SetMat2(const std::string &name, const glm::mat2 &value) const {
  glUniformMatrix2fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &value[0][0]);
}
void Shader::SetMat3(const std::string &name, const glm::mat3 &value) const {
  glUniformMatrix3fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &value[0][0]);
}
void Shader::SetMat4(const std::string &name, const glm::mat4 &value) const {
  glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &value[0][0]);
}

void Shader::CheckError(unsigned int shader, const std::string &type) {
  int success;
  char info_log[1024];
  if (type != "program") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, nullptr, info_log);
      throw std::string{info_log};
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, nullptr, info_log);
      throw std::string{info_log};
    }
  }
}

};  // namespace graphics