#include <iostream>
#include <random>
#include <cmath>

#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>
#include <iostream>
#include "game.h"
#include "gfx/program.h"
#include "gfx/vao.h"
#include "cubes.h"
#include "noise.h"

void Game::init() {
    cube_program.vertex({"cube.vs"}).fragment({"noise.glsl", "cube.fs"}).geometry({"cube.gs"}).compile();
    water_program.vertex({"cube.vs"}).fragment({"water.fs"}).geometry({"cube.gs"}).compile();
    background_tex.set_texture_size(window);
    render_tex.set_texture_size(window);
    std::default_random_engine generator;
    std::uniform_int_distribution<int> height(1,3);
    std::vector<Cubes::Instance> cube_instances;
    std::vector<Cubes::Instance> water_instances;
    for (int x = 0; x < 200; ++x) {
        for (int z = 0; z < 200; ++z) {
            float f = noise::perlin3d({x * 0.02, 0, z * 0.02}, 3, 0.5);
            int h = (int)(f * 20) + 20;
            for (int y = h - 1; y <= h; ++y) { 
                grid[x][y][z] = Block(true, (f < 0 ? 1 : f < 0.1 ? 2 : 3));
                cube_instances.emplace_back(Cubes::Instance(
                    glm::vec3(x, y, z),
                    grid[x][y][z].type
                ));
            }
            for (int y = h; y <= 12; ++y) {
                grid[x][y][z] = Block(false, 4);
                water_instances.emplace_back(Cubes::Instance(
                    glm::vec3(x, y, z),
                    grid[x][y][z].type
                ));
            }
        }
    }

    grid[20][40][20] = Block(true, 0);
    cube_instances.emplace_back(Cubes::Instance(
        glm::vec3(20, 40, 20),
        0
    ));

    cubes.vao.instances.set_data(cube_instances);
    water_cubes.vao.instances.set_data(water_instances);
}

void Game::update() {
    int window_w, window_h;
    glfwGetFramebufferSize(window, &window_w, &window_h);

    glm::mat4 view_matrix = glm::lookAt(glm::vec3(player_position), glm::vec3(player_position) + look, up);
    glm::mat4 projection_matrix = glm::perspective(
        glm::radians(80.f),
        ((float)window_w)/window_h,
        0.1f,
        1000.f
    );

    const glm::vec3 tangent = glm::cross(-look, up);
    glm::vec4 forward_step = (glm::vec4(look, 0) * forward_direction * movement_speed * (float)moving_forward);
    glm::vec4 side_step = (glm::vec4(tangent, 0) * movement_speed * sideways_direction* (float)moving_sideways);
    glm::vec4 gravity = glm::vec4(up, 0) * -movement_speed * (float)gravity_switch;

    glm::vec4 movement = forward_step +  side_step + gravity;
    int steps = glm::length(movement) * 1000;
    glm::vec4 step = glm::vec4(glm::vec3(movement) / (float)steps, 0);

    for (int i = 0; i < steps; ++i) {
        glm::ivec3 grid_pos_feet = gridWorld(step + player_position + glm::vec4(0,-1,0,0));
        glm::ivec3 grid_pos = gridWorld(step + player_position);
        glm::ivec3 old_grid_pos = gridWorld(player_position);

        if (grid[grid_pos.x][grid_pos.y][grid_pos.z].solid || 
            grid[grid_pos_feet.x][grid_pos_feet.y][grid_pos_feet.z].solid) {
            if (grid_pos != old_grid_pos) {
                // moved
                glm::vec3 normal_component = glm::proj(glm::vec3(step), glm::normalize(glm::vec3(old_grid_pos-grid_pos)));
                std::cout << glm::to_string(glm::vec3(old_grid_pos-grid_pos));
                std::cout << glm::to_string(glm::vec3(step)) << std::endl;
                step = step - glm::vec4(normal_component, 0);
            } else {
                // already in block
                std::cout << "FUCK" << std::endl;
                exit(0);
                // glm::vec3 dx = glm::vec3(player_position) - (glm::vec3(grid_pos) + glm::vec3(0.5));
                // player_position += glm::vec4(dx, 0);
            }
        }
        player_position += step;
    }

    glViewport(0, 0, window_w, window_h);
    glClearColor(0.f,0.f,0.f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    background_tex.bind_framebuffer();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, window_w, window_h);
        glClearColor(0.2f,0.2f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cube_program.use();
        glUniformMatrix4fv(cube_program.uniform_loc("projection"), 1, false, glm::value_ptr(projection_matrix));
        glUniformMatrix4fv(cube_program.uniform_loc("view"), 1, false, glm::value_ptr(view_matrix));
        cubes.draw();
    background_tex.unbind_framebuffer();

    glEnable(GL_DEPTH_TEST);
    display_quad.set_texture(background_tex.color_id).set_depth(background_tex.depth_id).draw();

    water_program.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, background_tex.color_id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, background_tex.depth_id);

    glUniform1i(water_program.uniform_loc("background"), 0);
    glUniform1i(water_program.uniform_loc("depth"), 1);
    glUniform1f(water_program.uniform_loc("z_near"), 0.1f);
    glUniform1f(water_program.uniform_loc("z_far"), 1000.f);
    glUniform1f(water_program.uniform_loc("time"), glfwGetTime());
    glUniform3fv(water_program.uniform_loc("look"), 1, glm::value_ptr(look));
    glUniformMatrix4fv(water_program.uniform_loc("projection"), 1, false, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(water_program.uniform_loc("view"), 1, false, glm::value_ptr(view_matrix));
    glUniform2i(water_program.uniform_loc("resolution"), window_w, window_h);
    
    water_cubes.draw();
}

void Game::updateOrientation() {
    if (glm::length(mouse_pos_vector) == 0) {
		return;}
	mouse_pos_vector.x *= -1.f;
    int window_w, window_h;
    glfwGetFramebufferSize(window, &window_w, &window_h);
     glm::mat4 projection_matrix = glm::perspective(
        glm::radians(80.f),
        ((float)window_w)/window_h,
        0.1f,
        100.f
    );

    glm::mat4 view_matrix = glm::lookAt(glm::vec3(player_position), glm::vec3(player_position) + look, up);

	const glm::vec3 world_vector = glm::vec3(glm::inverse(projection_matrix * view_matrix) * glm::vec4(mouse_pos_vector, 1., 1.));
	const glm::vec3 rotation_axis = glm::cross(look, world_vector);
	const glm::mat4 rotation = glm::rotate(mouse_speed * glm::length(mouse_pos_vector), rotation_axis);
	look = glm::vec3(rotation * glm::vec4(look, 0.));
}

 glm::ivec3 Game::gridWorld(const glm::vec3& pos) {
     return glm::ivec3((int)floor(pos.x), (int)floor(pos.y), (int)floor(pos.z));
 }
