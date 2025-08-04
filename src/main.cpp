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
#include <glm/glm.hpp>
#include <thread>

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
        .rgbx = 0xff000000,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {2,0,4},
        .shininess = 500,
        .rgbx = 0x0000ff00,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {-2,0,4},
        .shininess = 10,
        .rgbx = 0x00ff0000,
        .sphere = {
            .radius = 1.f,
        },
        .type = renderable_object::OBJECT_SPHERE
    },
    {
        .position = {0,5001,0},
        .shininess = 1000,
        .rgbx = 0xffff0000,
        .sphere = {
            .radius = 5000.f,
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
        .position = {2,1,0},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .2f,
            .type = renderable_object::LIGHT_POINT,
        },
        .type = renderable_object::OBJECT_LIGHT
    },
    {
        .direction = {1,4,4},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .6f,
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
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (!surface)
    {
        psdlerror("SDL_GetWindowSurface");
        return -1;
    }
    uint32_t bpp = surface->pitch/surface->w;
    for (int y = 0; y < surface->h; y++)
        memset(((char*)surface->pixels) + y*surface->pitch, 0, surface->w*bpp);

    renderer renderer = {surface->w,surface->h, [](void* userdata, const screen_coords& at, uint32_t rgbx) {
        SDL_Surface* surface = reinterpret_cast<SDL_Surface*>(userdata);
        uint8_t* fb8 = (uint8_t*)surface->pixels;
        uint32_t* fb32 = (uint32_t*)surface->pixels;
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
        // fb32[(surface->pitch/surface->format->BytesPerPixel)*at.y + at.x] = rgbx;
    }, surface, 0xffffffff};
    // renderable_object sphere = {};
    // sphere.rgbx.value = 0x00ff0000;
    // sphere.where.x = -5;
    // sphere.where.y = 0;
    // sphere.where.z = 0;
    // sphere.sphere.radius = 1.0f;
    // sphere.type = sphere.OBJECT_SPHERE;
    // renderer.append_object(&sphere);
    // renderer.set_camera_position({0,0,0,0});
    // renderable_object sphere2 = {};
    // sphere2.rgbx.value = 0x00ff0000;
    // sphere2.where.x = 5;
    // sphere2.where.y = 0;
    // sphere2.where.z = 0;
    // sphere2.sphere.radius = 1.0f;
    // sphere2.type = sphere.OBJECT_SPHERE;
    // renderer.append_object(&sphere2);
    for (int i = 0; i < sizeof(objects)/sizeof(*objects); i++)
        renderer.append_object(&objects[i]);

    bool quit = false;
    camera_position camera_pos = {};
    constexpr std::chrono::milliseconds target_fps_duration_ms = std::chrono::milliseconds(1/target_fps*1000);
    do {
        auto start = std::chrono::system_clock::now();
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
            }
            else if (event.type == SDL_QUIT)
            {
                quit = true;
                break;
            }
        }
        auto end = std::chrono::system_clock::now();

        SDL_UpdateWindowSurface(window);
        if ((end-start) >= target_fps_duration_ms)
            std::this_thread::sleep_for(target_fps_duration_ms - (end - start));
    } while(!quit);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
