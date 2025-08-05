/*
 * src/main.cpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_quit.h>

#include <cassert>
#include <chrono>
#include <thread>

#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string.h>
#include <stdio.h>

#include "renderer.hpp"

#define psdlerror(s) fprintf(stderr, "%s: %s\n", s, SDL_GetError())

constexpr int target_fps = 60;

using namespace raytracer;

renderable_object objects[] = {
    {
        .position = {0,1,3},
        .shininess = 500,
        .reflectiveness = .2f,
        .rgbx = 0xff000000,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {2,0,4},
        .shininess = 500,
        .reflectiveness = .3f,
        .rgbx = 0x0000ff00,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {-2,0,4},
        .shininess = 10,
        .reflectiveness = .4f,
        .rgbx = 0x00ff0000,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {0,1001,0},
        .shininess = 1000,
        .reflectiveness = .5f,
        .rgbx = 0xffff0000,
        .sphere = {
            .radius = 1000.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {0,0,0}, // disregarded
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .2f,
            .type = renderable_object::LIGHT_AMBIENT,
        },
        .type = renderable_object::OBJECT_LIGHT
    },
    {
        .position = {2,-1,0},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .6f,
            .type = renderable_object::LIGHT_POINT,
        },
        .type = renderable_object::OBJECT_LIGHT
    },
    {
        .direction = {1,4,4},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .2f,
            .type = renderable_object::LIGHT_DIRECTIONAL,
        },
        .type = renderable_object::OBJECT_LIGHT
    },
};

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        psdlerror("SDL_Init");
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Raytracer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        640, 480, 
        0);
    if (!window)
    {
        psdlerror("SDL_CreateWindow");
        return -1;
    }
    SDL_Surface* surfaces[2] = {SDL_GetWindowSurface(window),0};
    if (!surfaces[0])
    {
        psdlerror("SDL_GetWindowSurface");
        return -1;
    }
    surfaces[1] = SDL_CreateRGBSurface(0, surfaces[0]->w, surfaces[0]->h, 32, 0, 0,0,0);
    if (!surfaces[1])
    {
        psdlerror("SDL_CreateRGBSurface");
        return -1;
    }
    uint32_t bpp = surfaces[0]->format->BytesPerPixel;
    for (int y = 0; y < surfaces[0]->h; y++)
        memset(((char*)surfaces[0]->pixels) + y*surfaces[0]->pitch, 0, surfaces[0]->w*bpp);

    renderer renderer = {surfaces[1]->w,surfaces[1]->h, [](void* userdata, const screen_coords& at, uint32_t rgbx) {
        SDL_Surface* surface = reinterpret_cast<SDL_Surface**>(userdata)[1];
        // SDL_Surface* target_surface = reinterpret_cast<SDL_Surface**>(userdata)[0];
        uint8_t* fb8 = (uint8_t*)surface->pixels;
        assert(at.x < surface->w);
        assert(at.y < surface->h);

        uint8_t r = rgbx >> 24;
        uint8_t g = (rgbx >> 16) & 0xff;
        uint8_t b = (rgbx >> 8) & 0xff;

        fb8[surface->pitch * at.y + at.x*surface->format->BytesPerPixel + surface->format->Rshift/8] = r;
        fb8[surface->pitch * at.y + at.x*surface->format->BytesPerPixel + surface->format->Gshift/8] = g;
        fb8[surface->pitch * at.y + at.x*surface->format->BytesPerPixel + surface->format->Bshift/8] = b;
        if (surface->format->Amask)
            fb8[surface->pitch * at.y + at.x*surface->format->BytesPerPixel + surface->format->Ashift/8] = 0;
    }, surfaces, 0, 3};
    renderer.set_flush_buffers_cb([](void* userdata){
        SDL_Surface* surface = reinterpret_cast<SDL_Surface**>(userdata)[1];
        SDL_Surface* target_surface = reinterpret_cast<SDL_Surface**>(userdata)[0];
        SDL_BlitSurface(surface, nullptr, target_surface, nullptr);
    });
    for (int i = 0; i < sizeof(objects)/sizeof(*objects); i++)
        renderer.append_object(&objects[i]);

    bool quit = false;
    viewport_coords camera_pos = {};
    glm::mat3x3 camera_rot = glm::rotate(glm::mat4(1), 0.f, glm::vec3(0,0,1));
    renderer.set_camera_rotation(camera_rot);
    constexpr std::chrono::milliseconds target_fps_duration_ms = std::chrono::milliseconds(1/target_fps*1000);
    do {
        auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        renderer.render();

        SDL_Event event = {};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym) {
                    case 'w':
                        camera_pos.z++;
                        break;
                    case 's':
                        camera_pos.z--;
                        break;
                    case 'a':
                        camera_pos.x--;
                        break;
                    case 'd':
                        camera_pos.x++;
                        break;
                    case SDLK_UP:
                        camera_pos.y--;
                        break;
                    case SDLK_DOWN:
                        camera_pos.y++;
                        break;
                    default: break;
                }
                renderer.set_camera_position(camera_pos);
                renderer.render();
            }
            else if (event.type == SDL_QUIT)
            {
                quit = true;
                break;
            }
        }
        auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

        if ((end - start).count() > 5)
            printf("frame time = %ld ms\n", (end - start).count());
        SDL_UpdateWindowSurface(window);
        if ((end-start) >= target_fps_duration_ms)
            std::this_thread::sleep_for(target_fps_duration_ms - (end - start));
    } while(!quit);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
