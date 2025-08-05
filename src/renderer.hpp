/*
 * src/renderer.hpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#pragma once

#include <glm/ext/matrix_float3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <list>
#include <thread>
#include <utility>
#include <algorithm>

namespace raytracer {
    using viewport_coords = glm::vec3;
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
    inline static color color_multiply(color c, float val)
    {
        uint32_t r = std::clamp(((c >> 24) & 0xff) * val, 0.f,255.f);
        uint32_t g = std::clamp(((c >> 16) & 0xff) * val, 0.f,255.f);
        uint32_t b = std::clamp(((c >> 8) & 0xff) * val, 0.f,255.f);
        c = 0;
        c |= (r<<24);
        c |= (g<<16);
        c |= (b<<8);
        return c;
    }
    struct renderable_object {
        union {
            viewport_coords position;
            // For directional lights.
            viewport_coords direction;
        };
        float shininess = -1;
        float reflectiveness = 0;
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
            renderer(const renderer&) = delete;
            renderer(renderer&&) = delete;
            renderer(int screen_width, int screen_height, plot_pixel_cb cb, void* userdata, color bg_color, int recurse_limit);
    
            void render();
            inline void set_camera_position(const viewport_coords& new_pos) { set_mutated(); m_camera_position = new_pos; }
            inline void set_camera_rotation(const glm::mat3x3& rot) { set_mutated(); m_camera_rotation = rot; }
            inline void set_flush_buffers_cb(void(*cb)(void* userdata)) { set_mutated(); m_flush_buffers_cb = cb; }
            inline viewport_coords get_camera_position() { return m_camera_position; }
            inline glm::mat3x3 get_camera_rotation() { return m_camera_rotation; }
            inline void append_object(renderable_object* obj) { set_mutated(); m_objects.emplace_back(obj); }
            inline void remove_object(renderable_object* obj) { set_mutated(); m_objects.remove(obj); }
            inline void set_bg_color(color c) { set_mutated(); m_bg_color=c; }
            inline color get_bg_color() { return m_bg_color; }
            inline void set_mutated() { m_mutated = true; }

        private:
            std::list<renderable_object*> m_objects = {};
            viewport_coords m_camera_position = {};
            glm::mat3x3 m_camera_rotation = {};
            plot_pixel_cb m_plot_pixel = {};
            void(*m_flush_buffers_cb)(void* userdata) = nullptr;
            int m_recurse_limit = {};
            int m_screen_width = {};
            int m_screen_height = {};
            screen_coords m_screen_middle = {};
            canvas_coords m_screen_end = {};
            canvas_coords m_screen_start = {};
            glm::vec2 m_viewport_size = {};
            void* m_userdata = {};
            color m_bg_color = {};
            bool m_mutated = true;
            mutable bool m_needs_flush = false;
            bool m_workers_die = false;
            std::list<std::thread*> m_workers = {};

        private:
            static void render_worker(const renderer* This, int start_y, size_t nLines);
            screen_coords conv_canvas_screen(const canvas_coords& coords) const;
            color trace_ray(viewport_coords ray_coords, viewport_coords coords, float t_min, float t_max, int recurse_limit) const;
            bool ray_intersects_object(viewport_coords ray_coords, viewport_coords coords, float t_min, float t_max) const;
            float compute_lighting(viewport_coords intersection, glm::vec3 normal, glm::vec3 camera_distance, float shininess) const;
            std::pair<float /* t1 */, float /* t2 */> intersect_ray_sphere(viewport_coords ray_coords, viewport_coords coords, const renderable_object& sphere) const;
    };
}