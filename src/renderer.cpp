/*
 * src/renderer.cpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include <list>

#include "renderer.hpp"

namespace raytracer {
    renderer::renderer(int screen_width, int screen_height, plot_pixel_cb cb, void* userdata, color bg_color, int recurse_limit)
        : m_screen_height(screen_height), m_screen_width(screen_width),
          m_plot_pixel(cb), m_userdata(userdata),
          m_bg_color(bg_color),
          m_recurse_limit(recurse_limit)
    {
        m_screen_middle.x = screen_width/2;
        m_screen_middle.y = screen_height/2;
        m_screen_end.x = m_screen_middle.x;
        m_screen_end.y = m_screen_middle.y;
        m_screen_start.x = -m_screen_middle.x;
        m_screen_start.y = -m_screen_middle.y;
        m_viewport_size.x = 1;
        m_viewport_size.y = 1;
    }

    void renderer::render()
    {
        if (!m_mutated) return;
        canvas_coords i = m_screen_start;
        const float d = 1;
        for (; i.y < m_screen_end.y; i.y++)
        {
            for (; i.x < m_screen_end.x; i.x++)
            {
                viewport_coords coords = {};
                coords.x = i.x * (m_viewport_size.x/m_screen_end.x);
                coords.y = i.y * (m_viewport_size.y/m_screen_end.y);
                coords.z = d;
                coords = coords * m_camera_rotation;

                color c = trace_ray(m_camera_position, coords, 1, INFINITY, m_recurse_limit);
                m_plot_pixel(m_userdata, conv_canvas_screen(i), c);                
            }
            i.x = m_screen_start.x;
        }
        m_mutated = false;
    }
    
#define in_range(v, min, max) (((v) >= (min)) && ((v) < (max)))

    bool renderer::ray_intersects_object(viewport_coords ray_coords, viewport_coords coords, float t_min, float t_max)
    {
        for (auto &object : m_objects)
        {
            float t1 = INFINITY, t2 = INFINITY;
            switch (object->type)
            {
                case renderable_object::OBJECT_SPHERE:
                {
                    auto p = intersect_ray_sphere(ray_coords, coords, *object);
                    t1 = p.first;
                    t2 = p.second;
                    break;
                }
                case renderable_object::OBJECT_LIGHT: continue;
                default: assert(!"unimplemented object type");
            }
            if (in_range(t1, t_min, t_max))
                return true;
            if (in_range(t2, t_min, t_max))
                return true;
        }
        return false;
    }

    color renderer::trace_ray(viewport_coords ray_coords, viewport_coords coords, float t_min, float t_max, int recurse_limit)
    {
        float closest_t = INFINITY;
        renderable_object* closest_object = nullptr;
        for (auto &object : m_objects)
        {
            float t1 = INFINITY, t2 = INFINITY;
            switch (object->type)
            {
                case renderable_object::OBJECT_SPHERE:
                {
                    auto p = intersect_ray_sphere(ray_coords, coords, *object);
                    t1 = p.first;
                    t2 = p.second;
                    break;
                }
                case renderable_object::OBJECT_LIGHT: continue;
                default: assert(!"unimplemented object type");
            }

            if (in_range(t1, t_min, t_max) && t1 < closest_t)
            {
                closest_t = t1;
                closest_object = object;
            }
            if (in_range(t2, t_min, t_max) && t2 < closest_t)
            {
                closest_t = t2;
                closest_object = object;
            }
        }

        if (!closest_object)
            return m_bg_color;

        viewport_coords intersection_coords = closest_t * coords;
        glm::vec3 normal = {};
        if (closest_object->type == renderable_object::OBJECT_SPHERE)
        {
            normal = intersection_coords - closest_object->position;
            normal /= glm::length(normal);
        }
        else
            assert(!"unknown object type");
        color local_color = closest_object->rgbx;
        float n = compute_lighting(intersection_coords, normal, -coords, closest_object->shininess);
        local_color = color_multiply(local_color, n);
        
        if (recurse_limit <= 0 || closest_object->reflectiveness <= 0)
            return local_color;

        glm::vec3 reflected_ray = 2.f * normal * glm::dot(normal, -coords) - (-coords);
        color reflected_color = trace_ray(intersection_coords, reflected_ray, 0.001, INFINITY, recurse_limit - 1);
        local_color = color_multiply(local_color, (1.f-closest_object->reflectiveness));
        reflected_color = color_multiply(reflected_color, closest_object->reflectiveness);
        return local_color + reflected_color;
    }

    std::pair<float /* t1 */, float /* t2 */> renderer::intersect_ray_sphere(viewport_coords ray_coords, viewport_coords coords, const renderable_object& sphere)
    {
        glm::vec3 co = ray_coords - sphere.position;
        float a = glm::dot(coords,coords);
        float b = 2.f*glm::dot(co, coords);
        float c = glm::dot(co, co) - sphere.sphere.radius*sphere.sphere.radius;

        float discriminant = b*b - 4.f * a * c;
        if (discriminant < 0) { return {INFINITY,INFINITY}; }
    
        float t1 = (-b + (float)sqrt(discriminant)) / (2.f*a);
        float t2 = (-b - (float)sqrt(discriminant)) / (2.f*a);
        return {t1,t2};
    }

    float renderer::compute_lighting(viewport_coords intersection, glm::vec3 normal, glm::vec3 camera_distance, float shininess)
    {
        float n = 0;
        for (auto& light : m_objects)
        {
            if (light->type != renderable_object::OBJECT_LIGHT) continue;
            switch (light->light.type) {
                case renderable_object::LIGHT_AMBIENT:
                    n += light->light.intensity;
                    break;
                case renderable_object::LIGHT_POINT:
                case renderable_object::LIGHT_DIRECTIONAL:
                {
                    glm::vec3 direction = {};
                    float t_max = 0;
                    if (light->light.type == renderable_object::LIGHT_POINT)
                    {
                        direction = light->position - intersection;
                        t_max = 1;
                    }
                    else
                    {
                        direction = light->direction;
                        t_max = INFINITY;
                    }
                    if (ray_intersects_object(intersection, direction, 0.001, t_max))
                        continue;
                    float dot_l = glm::dot(normal, direction);
                    if (dot_l > 0) n += light->light.intensity * dot_l / (glm::length(normal)*glm::length(direction));
                    
                    if (shininess != -1)
                    {
                        glm::vec3 r = 2.f * normal * glm::dot(normal, direction) - direction;
                        float r_dot_v = glm::dot(r,camera_distance);
                        if (r_dot_v > 0)
                            n += light->light.intensity * pow(r_dot_v / (glm::length(r) * glm::length(camera_distance)), shininess);
                    }

                    break;
                }
            }
        }

        return n;
    }

    screen_coords renderer::conv_canvas_screen(const canvas_coords& coords)
    {
        screen_coords ret = {};
        ret.x = m_screen_middle.x+coords.x;
        ret.y = m_screen_middle.y+coords.y;
        return ret;
    }
    
}