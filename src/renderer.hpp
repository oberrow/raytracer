/*
 * src/renderer.hpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <list>
#include <utility>

namespace raytracer {
    using viewport_coords = glm::vec3;
    using camera_position = glm::vec4; // x,y,z,camera angle
    struct canvas_coords
    {
        int x, y;
    };
    struct screen_coords
    {
        unsigned int x, y;
    };
    typedef void(*plot_pixel_cb)(void* userdata, const screen_coords& at, uint32_t rgbx);
    using color = uint32_t;
    struct renderable_object {
        union {
            viewport_coords position;
            // For directional lights.
            viewport_coords direction;
        };
        float shininess = -1;
        color rgbx;
        union {
            struct {
                float radius;
            } sphere;
            struct {
                float intensity;
                uint8_t type;
            } light;
        };
        enum {
            OBJECT_INVALID = 0,
            OBJECT_SPHERE = 1,
            OBJECT_LIGHT = 2,
        } type;
        enum {
            LIGHT_AMBIENT,
            LIGHT_POINT,
            LIGHT_DIRECTIONAL,
        };
    };
    
    class renderer {
        public:
            renderer() = delete;
            renderer(int screen_width, int screen_height, plot_pixel_cb cb, void* userdata, color bg_color);
    
            void render();
            inline void set_camera_position(const camera_position& new_pos) { m_camera_position = new_pos; }
            inline camera_position get_camera_position() { return m_camera_position; }
            inline void append_object(renderable_object* obj) { m_objects.emplace_back(obj); }
            inline void remove_object(renderable_object* obj) { m_objects.remove(obj); }
            inline void set_bg_color(color c) { m_bg_color=c; }
            inline color get_bg_color(color c) { return m_bg_color; }

        private:
            std::list<renderable_object*> m_objects;
            camera_position m_camera_position;
            plot_pixel_cb m_plot_pixel;
            int m_screen_width;
            int m_screen_height;
            screen_coords m_screen_middle;
            canvas_coords m_screen_end;
            canvas_coords m_screen_start;
            glm::vec2 m_viewport_size;
            void* m_userdata;
            color m_bg_color;
            screen_coords conv_canvas_screen(const canvas_coords& coords);
            color trace_ray(viewport_coords ray_coords, viewport_coords coords, float t_min, float t_max);
            float compute_lighting(viewport_coords intersection, glm::vec3 normal, glm::vec3 camera_distance, float shininess);
            std::pair<float /* t1 */, float /* t2 */> intersect_ray_sphere(viewport_coords ray_coords, viewport_coords coords, const renderable_object& sphere);
    };
}