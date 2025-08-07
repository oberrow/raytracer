/*
 * src/main-obos.cpp
 *
 * Copyright (c) 2025 Omar Berrow
*/

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/select.h>
#include <thread>
#include <mutex>

#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <obos/syscall.h>
#include <obos/error.h>

#include "renderer.hpp"
#include "scene.hpp"

constexpr int target_fps = 60;

using namespace raytracer;

#ifndef __obos__
#   error Only works on OBOS
#endif

struct fb_mode {
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint16_t format; // See OBOS_FB_FORMAT_*
    uint8_t bpp; 
};

struct framebuffer {
    union {
        void* buff;
        uint8_t* buff8;
    };
    struct fb_mode mode;
    int red_shift;
    int green_shift;
    int blue_shift;
    int x_shift;
    bool has_x_shift = false;
};

#define OBOS_FB_FORMAT_RGB888 1
#define OBOS_FB_FORMAT_BGR888 2
#define OBOS_FB_FORMAT_RGBX8888 3
#define OBOS_FB_FORMAT_XRGB8888 4

int main()
{
    int fb0_fd = open("/dev/fb0", O_RDWR);
    if (fb0_fd < 0)
    {
        perror("open(/dev/fb0, O_RDWR)");
        return -1;
    }
    struct stat st = {};
    fstat(fb0_fd, &st);

    fb_mode mode = {};
    framebuffer fb0 = {};
    if (ioctl(fb0_fd, 1 /* query framebuffer info */, &mode))
    {
        perror("ioctl");
        return -1;
    }
    fb0.mode = mode;
//    fb0.buff = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb0_fd, 0);
    struct vma_alloc_userspace_args
    {
        uint32_t prot;
        uint32_t flags;
        handle file;
        uintptr_t offset;
    } mmap_args = {};
    mmap_args.flags |= (1<<11) /* framebuffer, default is MAP_SHARED */;
    mmap_args.file = fb0_fd;

    obos_status status = OBOS_STATUS_SUCCESS;
    fb0.buff = (void*)syscall5(Sys_VirtualMemoryAlloc, HANDLE_CURRENT, nullptr, st.st_size, &mmap_args, &status);
    if (obos_is_error(status))
    {
        fprintf(stderr, "Sys_VirtualMemoryAlloc returned obos_status %d\n", status);
        return -1;
    }

    switch (fb0.mode.format) {
        case OBOS_FB_FORMAT_RGB888:
            fb0.red_shift = 16;
            fb0.green_shift = 8;
            fb0.blue_shift = 0;
            fb0.has_x_shift = false;
            break;
        case OBOS_FB_FORMAT_BGR888:
            fb0.red_shift = 0;
            fb0.green_shift = 8;
            fb0.blue_shift = 16;
            fb0.has_x_shift = false;
            break;
        case OBOS_FB_FORMAT_RGBX8888:
            fb0.red_shift = 24;
            fb0.green_shift = 16;
            fb0.blue_shift = 8;
            fb0.x_shift = 0;
            break;
        case OBOS_FB_FORMAT_XRGB8888:
            fb0.red_shift = 16;
            fb0.green_shift = 8;
            fb0.blue_shift = 0;
            fb0.x_shift = 24;
            break;
        default: abort();
    }
    if (fb0.mode.width > 640)
        fb0.mode.width = 640;
    if (fb0.mode.height > 480)
        fb0.mode.height = 480;
    printf("%s: Framebuffer is %dx%dx%d\n", __func__, fb0.mode.width, fb0.mode.height, fb0.mode.bpp);
    printf("%s: Mapped framebuffer at %p.\n", __func__, fb0.buff);

    renderer renderer = {
    static_cast<int>(fb0.mode.width), static_cast<int>(fb0.mode.height),
        [](void* userdata, const screen_coords& at, uint32_t rgbx) {
            framebuffer *fb = (framebuffer*)userdata;
            uint8_t r = rgbx >> 24;
            uint8_t g = (rgbx >> 16) & 0xff;
            uint8_t b = (rgbx >> 8) & 0xff;

            fb->buff8[fb->mode.pitch * at.y + at.x*(fb->mode.bpp/8) + fb->red_shift/8] = r;
            fb->buff8[fb->mode.pitch * at.y + at.x*(fb->mode.bpp/8) + fb->green_shift/8] = g;
            fb->buff8[fb->mode.pitch * at.y + at.x*(fb->mode.bpp/8) + fb->blue_shift/8] = b;
            if (fb->has_x_shift)
                fb->buff8[fb->mode.pitch * at.y + at.x*(fb->mode.bpp/8) + fb->x_shift/8] = 0;
        }, &fb0, s_bg_color, 3
    };
    for (int i = 0; i < sizeof(objects)/sizeof(*objects); i++)
        renderer.append_object(&objects[i]);

    bool quit = false;
    viewport_coords camera_pos = {};
    glm::mat3x3 camera_rot = glm::rotate(glm::mat4(1), 0.f, glm::vec3(0,0,1));
    renderer.set_camera_rotation(camera_rot);
    constexpr std::chrono::milliseconds target_fps_duration_ms = std::chrono::milliseconds((int)(1.f/(float)target_fps*1000.f));
    struct timespec timeout = {};
    timeout.tv_nsec = 0;
    timeout.tv_sec = 0;
    fd_set stdin_set;
    FD_ZERO(&stdin_set);
    FD_SET(0, &stdin_set);
    struct termios term = {};
    tcgetattr(0, &term);
    cfmakeraw(&term);
    term.c_lflag |= ISIG;
    tcsetattr(0, 0, &term);
    do {
        auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        renderer.render();
        char c = 0;
        if (pselect(1, &stdin_set, nullptr, nullptr, &timeout, nullptr))
        {
            read(0, &c, 1);
            FD_SET(0, &stdin_set);
        }
        bool changed_camera_pos = false;
        switch (c) {
            case 0: break;
            case 'w':
                camera_pos.z++;
                changed_camera_pos = true;
                break;
            case 's':
                camera_pos.z--;
                changed_camera_pos = true;
                break;
            case 'a':
                camera_pos.x--;
                changed_camera_pos = true;
                break;
            case 'd':
                camera_pos.x++;
                changed_camera_pos = true;
                break;
            case '\x1b': quit = true; break;
        }
        c = 0;
        if (changed_camera_pos)
            renderer.set_camera_position(camera_pos);
        auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

        // if ((end - start).count() > 5)
        //     printf("frame time = %ld ms\n", (end - start).count());
        if ((end-start) >= target_fps_duration_ms)
            std::this_thread::sleep_for(target_fps_duration_ms - (end - start));
    } while(!quit);
}
