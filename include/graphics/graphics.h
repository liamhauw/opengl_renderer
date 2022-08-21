#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include <vector>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

namespace graphics {

extern std::string root_directory;

class Camera;
extern Camera camera;

extern float last_cursor_xpos;
extern float last_cursor_ypos;
extern bool first_cursor;

extern float current_time;
extern float last_time;
extern float delta_time;

extern bool enable_camera_move;

extern unsigned sphere_vao;
extern unsigned sphere_index_count;
extern unsigned cube_vao;
extern unsigned quad_vao;

unsigned LoadTexture(const std::string &path);
std::vector<std::vector<unsigned>> LoadPbrTexture(const char *material[],
                                                  unsigned count);

void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mods);
void CursorPosCallback(GLFWwindow *window, double xposition, double yposition);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ProcessInput(GLFWwindow *window);

void RenderSphere();
void RenderCube();
void RenderQuad();

void GlCheckError(const std::string &file, int line);

enum CameraDirection { kForward, kBackward, kLeft, kRight };
class Camera {
 public:
  Camera(glm::vec3 position, glm::vec3 world_up = glm::vec3{0.0f, 1.0f, 0.0f},
         float yaw = -90.0f, float pitch = 0.0f);
  glm::mat4 GetViewMatrix();
  void ProcessKey(CameraDirection direction, float delta_time);
  void ProcessCursorPos(float xoffset, float yoffset,
                        bool constrain_pitch = true);
  void ProcessScroll(float yoffset);

  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 world_up_;
  float yaw_;
  float pitch_;
  float speed_;
  float sensitivity_;
  float yfov_;

 private:
  void UpdateCamera();
};

class Shader {
 public:
  Shader(const std::string &vertex_shader, const std::string &fragment_shader,
         const std::string &geometry_shader = std::string{});
  void UseProgram();
  void SetBool(const std::string &name, bool value) const;
  void SetInt(const std::string &name, int value) const;
  void SetFloat(const std::string &name, float value) const;
  void SetVec2(const std::string &name, const glm::vec2 &value) const;
  void SetVec2(const std::string &name, float x, float y) const;
  void SetVec3(const std::string &name, const glm::vec3 &value) const;
  void SetVec3(const std::string &name, float x, float y, float z) const;
  void SetVec4(const std::string &name, const glm::vec4 &value) const;
  void SetVec4(const std::string &name, float x, float y, float z, float w);
  void SetMat2(const std::string &name, const glm::mat2 &value) const;
  void SetMat3(const std::string &name, const glm::mat3 &value) const;
  void SetMat4(const std::string &name, const glm::mat4 &value) const;

  unsigned program;

 private:
  void CheckError(unsigned shader, const std::string &type);
};

};  // namespace graphics

#endif