#include <iostream>
#include <string>
#include <vector>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "graphics/graphics.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "stb/stb_image.h"

using namespace graphics;

void Graphics();

int main() {
  try {
    Graphics();
  } catch (const std::string &e) {
    std::cout << "exception: " << e << std::endl;
    return -1;
  }
  return 0;
}

void Graphics() {
  // glfw
  // ----
  glfwSetErrorCallback(ErrorCallback);
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  unsigned kWindowWidth{1280};
  unsigned kWindowHeight{720};
  GLFWwindow *window = glfwCreateWindow(kWindowWidth, kWindowHeight, "graphics",
                                        nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetScrollCallback(window, ScrollCallback);
  glfwSetKeyCallback(window, KeyCallback);

  // glew
  // ----
  GLenum glew_initilize{glewInit()};
  if (glew_initilize != GLEW_OK) {
    throw std::string{"fail to init glew"};
  }

  // imgui
  // -----
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io{ImGui::GetIO()};
  io.IniFilename = nullptr;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  // opengl
  // ------
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  Shader radiance_shader{"cubemap.vs", "cubemap_radiance.fs"};
  Shader irradiance_shader{"cubemap.vs", "cubemap_irradiance.fs"};
  Shader prefilter_shader{"cubemap.vs", "cubemap_prefilter.fs"};
  Shader brdf_shader{"brdf.vs", "brdf.fs"};
  Shader pbr_shader{"pbr.vs", "pbr.fs"};
  Shader background_shader{"background.vs", "background.fs"};

  glm::mat4 cubemap_projection{
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f)};
  std::vector<glm::mat4> cubemap_view{
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{1.0f, 0.0f, 0.0f},
                  glm::vec3{0.0f, -1.0f, 0.0f}),
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{-1.0f, 0.0f, 0.0f},
                  glm::vec3{0.0f, -1.0f, 0.0f}),
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f},
                  glm::vec3{0.0f, 0.0f, 1.0f}),
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, -1.0f, 0.0f},
                  glm::vec3{0.0f, 0.0f, -1.0f}),
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f},
                  glm::vec3{0.0f, -1.0f, 0.0f}),
      glm::lookAt(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f},
                  glm::vec3{0.0f, -1.0f, 0.0f})};

  unsigned capture_fbo;
  unsigned capture_rbo;
  {
    glGenFramebuffers(1, &capture_fbo);
    glGenRenderbuffers(1, &capture_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, capture_rbo);
  }

  unsigned radiance_texture;
  {
    stbi_set_flip_vertically_on_load(true);
    int x, y, channel;
    float *hdr_image{stbi_loadf(
        (std::string{root_directory} + "/resource/texture/hdr/alexs_apt_2k.hdr")
            .c_str(),
        &x, &y, &channel, 0)};
    unsigned int hdr_texture;
    if (hdr_image) {
      glGenTextures(1, &hdr_texture);
      glBindTexture(GL_TEXTURE_2D, hdr_texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, x, y, 0, GL_RGB, GL_FLOAT,
                   hdr_image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      stbi_image_free(hdr_image);
    } else {
      throw std::string{"failed to load hdr image"};
    }

    glGenTextures(1, &radiance_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, radiance_texture);
    for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512,
                   0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glViewport(0, 0, 512, 512);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr_texture);
    radiance_shader.UseProgram();
    radiance_shader.SetInt("equirectangular_texture", 0);
    radiance_shader.SetMat4("projection", cubemap_projection);
    for (unsigned int i = 0; i < 6; ++i) {
      radiance_shader.SetMat4("view", cubemap_view[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             radiance_texture, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      RenderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, radiance_texture);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }

  unsigned irradiance_texture;
  {
    glGenTextures(1, &irradiance_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_texture);
    for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0,
                   GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glViewport(0, 0, 32, 32);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, radiance_texture);
    irradiance_shader.UseProgram();
    irradiance_shader.SetInt("environment_texture", 0);
    irradiance_shader.SetMat4("projection", cubemap_projection);
    for (unsigned int i = 0; i < 6; ++i) {
      irradiance_shader.SetMat4("view", cubemap_view[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             irradiance_texture, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      RenderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  unsigned prefilter_texture;
  {
    glGenTextures(1, &prefilter_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_texture);
    for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128,
                   0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, radiance_texture);

    prefilter_shader.UseProgram();
    prefilter_shader.SetInt("environment_texture", 0);
    prefilter_shader.SetMat4("projection", cubemap_projection);
    unsigned max_mip_level{5};
    for (unsigned mip = 0; mip < max_mip_level; ++mip) {
      unsigned mip_width = 128 * std::pow(0.5, mip);
      unsigned mip_height = 128 * std::pow(0.5, mip);
      glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width,
                            mip_height);
      glViewport(0, 0, mip_width, mip_height);

      float roughness = (float)mip / (float)(max_mip_level - 1);
      prefilter_shader.SetFloat("roughness", roughness);
      for (unsigned i = 0; i < 6; ++i) {
        prefilter_shader.SetMat4("view", cubemap_view[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               prefilter_texture, mip);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
      }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  unsigned int brdf_texture;
  {
    glGenTextures(1, &brdf_texture);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           brdf_texture, 0);
    glViewport(0, 0, 512, 512);
    brdf_shader.UseProgram();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  int window_width, window_height;
  glfwGetFramebufferSize(window, &window_width, &window_height);
  glViewport(0, 0, window_width, window_height);

  std::vector<glm::vec3> light_position{
      glm::vec3{-10.0f, 10.0f, 10.0f},
      glm::vec3{10.0f, 10.0f, 10.0f},
      glm::vec3{-10.0f, -10.0f, 10.0f},
      glm::vec3{10.0f, -10.0f, 10.0f},
  };
  std::vector<glm::vec3> light_color{
      glm::vec3{300.0f, 300.0f, 300.0f}, glm::vec3{300.0f, 300.0f, 300.0f},
      glm::vec3{300.0f, 300.0f, 300.0f}, glm::vec3{300.0f, 300.0f, 300.0f}};

  pbr_shader.UseProgram();
  pbr_shader.SetInt("normal_texture", 0);
  pbr_shader.SetInt("albedo_texture", 1);
  pbr_shader.SetInt("metallic_texture", 2);
  pbr_shader.SetInt("roughness_texture", 3);
  pbr_shader.SetInt("ao_texture", 4);
  pbr_shader.SetInt("irradiance_texture", 5);
  pbr_shader.SetInt("prefilter_texture", 6);
  pbr_shader.SetInt("brdf_texture", 7);
  for (unsigned i = 0; i < light_position.size(); ++i) {
    pbr_shader.SetVec3("light_position[" + std::to_string(i) + "]",
                       light_position[i]);
    pbr_shader.SetVec3("light_color[" + std::to_string(i) + "]",
                       light_color[i]);
  }

  background_shader.UseProgram();
  background_shader.SetInt("environment_texture", 0);

  // gui
  // ---
  int model_value{0};
  float scale_value{1.0f};
  glm::vec3 translation_value{0.0f, 0.0f, 0.0f};
  glm::vec3 clear_color_value{0.5f, 0.7f, 0.7f};
  bool punctual_light_value{true};
  bool image_based_light_value{true};
  int pbr_material_value{0};
  const char *pbr_material[]{"rusted_iron", "gold",    "loose_tablecloth",
                             "grass",       "plastic", "wall"};
  unsigned pbr_material_count{sizeof(pbr_material) / sizeof(const char *)};
  std::vector<std::vector<unsigned>> pbr_texture{
      LoadPbrTexture(pbr_material, pbr_material_count)};
  bool background_value{true};

  while (!glfwWindowShouldClose(window)) {
    // timer
    // -----
    current_time = glfwGetTime();
    delta_time = current_time - last_time;
    last_time = current_time;

    // input
    // -----
    ProcessInput(window);

    // imgui
    // -----
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // demo gui
    // ImGui::ShowDemoWindow();

    // opengl
    const ImGuiViewport *main_viewport{ImGui::GetMainViewport()};
    ImGui::SetNextWindowSize(ImVec2{300, 300});
    ImGui::Begin("real-time rendering");
    ImGui::LabelText("label", "value");
    const char *model_items[]{"sphere", "cube", "quad"};
    ImGui::Combo("model", &model_value, model_items, IM_ARRAYSIZE(model_items));
    ImGui::SliderFloat("scale", &scale_value, 0.1f, 2.0f, "%.3f");
    ImGui::SliderFloat3("translate",
                        reinterpret_cast<float *>(&translation_value), -1.0f,
                        1.0f, "%.3f");
    ImGui::ColorEdit3("clear color",
                      reinterpret_cast<float *>(&clear_color_value));
    ImGui::Combo("pbr_material", &pbr_material_value, pbr_material,
                 IM_ARRAYSIZE(pbr_material));
    
    ImGui::Checkbox("punctual light", &punctual_light_value);
    ImGui::Checkbox("image based light", &image_based_light_value);
    ImGui::Checkbox("background", &background_value);
    ImGui::End();

    // opengl
    // ------
    glClearColor(clear_color_value.r, clear_color_value.g, clear_color_value.b,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model{1.0f};
    model = glm::translate(model, glm::vec3{translation_value});
    model = glm::scale(model, glm::vec3{scale_value});
    glm::mat4 view{camera.GetViewMatrix()};
    glm::mat4 projection{glm::perspective(
        glm::radians(camera.yfov_),
        static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight),
        0.1f, 100.0f)};

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pbr_texture[pbr_material_value][0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pbr_texture[pbr_material_value][1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pbr_texture[pbr_material_value][2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, pbr_texture[pbr_material_value][3]);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, pbr_texture[pbr_material_value][4]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_texture);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_texture);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);

    pbr_shader.UseProgram();
    pbr_shader.SetMat4("model", model);
    pbr_shader.SetMat4("view", view);
    pbr_shader.SetMat4("projection", projection);
    pbr_shader.SetVec3("camera_position", camera.position_);
    pbr_shader.SetBool("punctual_light", punctual_light_value);
    pbr_shader.SetBool("image_based_light", image_based_light_value);

    if (model_value == 0) {
      RenderSphere();
    } else if (model_value == 1) {
      RenderCube();
    } else if (model_value == 2) {
      RenderQuad();
    }

    if (background_value) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, radiance_texture);
      background_shader.UseProgram();
      background_shader.SetMat4("view", view);
      background_shader.SetMat4("projection", projection);
      RenderCube();
    }

    // imgui
    // -----
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // glfw
    // ----
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // imgui
  // -----
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // glfw
  // ----
  glfwDestroyWindow(window);
  glfwTerminate();
}
