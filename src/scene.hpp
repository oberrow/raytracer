/*
 * src/scene.hpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#pragma once

#include "renderer.hpp"

static raytracer::renderable_object objects[] = {
    {
        .position = {0,1,3},
        .shininess = 500,
        .reflectiveness = .2f,
        .rgbx = 0xff000000,
        .sphere = {
            .radius = 1.f,
        },
        .type = raytracer::renderable_object::OBJECT_SPHERE
    },
    {
        .position = {2,0,4},
        .shininess = 500,
        .reflectiveness = .3f,
        .rgbx = 0x0000ff00,
        .sphere = {
            .radius = 1.f,
        },
        .type = raytracer::renderable_object::OBJECT_SPHERE
    },
    {
        .position = {-2,0,4},
        .shininess = 10,
        .reflectiveness = .4f,
        .rgbx = 0x00ff0000,
        .sphere = {
            .radius = 1.f,
        },
        .type = raytracer::renderable_object::OBJECT_SPHERE
    },
    {
        .position = {0,1001,0},
        .shininess = 1000,
        .reflectiveness = .5f,
        .rgbx = 0xffff0000,
        .sphere = {
            .radius = 1000.f,
        },
        .type = raytracer::renderable_object::OBJECT_SPHERE
    },
    {
        .position = {0,0,0}, // disregarded
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .2f,
            .type = raytracer::renderable_object::LIGHT_AMBIENT,
        },
        .type = raytracer::renderable_object::OBJECT_LIGHT
    },
    {
        .position = {2,-1,0},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .6f,
            .type = raytracer::renderable_object::LIGHT_POINT,
        },
        .type = raytracer::renderable_object::OBJECT_LIGHT
    },
    {
        .direction = {1,4,4},
        .rgbx = 0xffffffff,
        .light = {
            .intensity = .2f,
            .type = raytracer::renderable_object::LIGHT_DIRECTIONAL,
        },
        .type = raytracer::renderable_object::OBJECT_LIGHT
    },
};