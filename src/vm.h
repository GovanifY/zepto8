//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>

#include "zepto8.h"
#include "cart.h"

namespace z8
{

using lol::array;
using lol::u8vec4;

class player;

class vm : public lol::LuaLoader,
           public lol::LuaObject
{
    friend class z8::player;

public:
    vm();
    ~vm();

    void load(char const *name);
    void run();
    void step(float seconds);

    uint8_t *get_mem(int offset = 0) { return &m_memory[offset]; }
    uint8_t const *get_mem(int offset = 0) const { return &m_memory[offset]; }

    void render(lol::u8vec4 *screen) const;
    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 128),
                    uint8_t const *prev_screen = nullptr) const;

    void button(int index, int state) { m_buttons[1][index] = state; }
    void mouse(lol::ivec2 coords, int buttons) { m_mouse = lol::ivec3(coords, buttons); }

    static const lol::LuaObjectLibrary* GetLib();
    static vm* New(lua_State* l, int arg_nb);

private:
    void set_this(lua_State *l);
    static vm* get_this(lua_State *l);
    static void hook(lua_State *l, lua_Debug *ar);

    struct api
    {
        // System
        static int run(lua_State *l);
        static int menuitem(lua_State *l);
        static int cartdata(lua_State *l);
        static int reload(lua_State *l);
        static int peek(lua_State *l);
        static int poke(lua_State *l);
        static int memcpy(lua_State *l);
        static int memset(lua_State *l);
        static int dget(lua_State *l);
        static int dset(lua_State *l);
        static int stat(lua_State *l);
        static int printh(lua_State *l);

        // I/O
        static int update_buttons(lua_State *l);
        static int btn(lua_State *l);
        static int btnp(lua_State *l);

        // Text
        static int cursor(lua_State *l);
        static int print(lua_State *l);

        // Maths
        static int max(lua_State *l);
        static int min(lua_State *l);
        static int mid(lua_State *l);
        static int flr(lua_State *l);
        static int cos(lua_State *l);
        static int sin(lua_State *l);
        static int atan2(lua_State *l);
        static int sqrt(lua_State *l);
        static int abs(lua_State *l);
        static int sgn(lua_State *l);

        static int rnd(lua_State *l);
        static int srand(lua_State *l);

        static int band(lua_State *l);
        static int bor(lua_State *l);
        static int bxor(lua_State *l);
        static int bnot(lua_State *l);
        static int shl(lua_State *l);
        static int shr(lua_State *l);

        // Graphics
        static int camera(lua_State *l);
        static int circ(lua_State *l);
        static int circfill(lua_State *l);
        static int clip(lua_State *l);
        static int cls(lua_State *l);
        static int color(lua_State *l);
        static int fget(lua_State *l);
        static int fset(lua_State *l);
        static int line(lua_State *l);
        static int map(lua_State *l);
        static int mget(lua_State *l);
        static int mset(lua_State *l);
        static int pal(lua_State *l);
        static int palt(lua_State *l);
        static int pget(lua_State *l);
        static int pset(lua_State *l);
        static int rect(lua_State *l);
        static int rectfill(lua_State *l);
        static int sget(lua_State *l);
        static int sset(lua_State *l);
        static int spr(lua_State *l);
        static int sspr(lua_State *l);

        // Sound
        static int music(lua_State *l);
        static int sfx(lua_State *l);

        // Deprecated
        static int time(lua_State *l);
    };

private:
    int getpixel(int x, int y);
    void setpixel(int x, int y, int color);

    void hline(int x1, int x2, int y, int color);
    void vline(int x, int y1, int y2, int color);

    int getspixel(int x, int y);
    void setspixel(int x, int y, int color);

    void getaudio(int channel, void *buffer, int bytes);

private:
    uint8_t m_memory[SIZE_MEMORY];
    lol::Image m_font;
    cart m_cart;

    // Graphics
    uint8_t m_color;
    lol::ivec2 m_camera, m_cursor;
    lol::ibox2 m_clip;
    uint8_t m_pal[2][16], m_palt[16];

    // Input
    int m_buttons[2][64];
    lol::ivec3 m_mouse;

    // Audio
    struct channel
    {
        channel();

        int m_sfx;
        float m_offset, m_phi;
#if DEBUG_EXPORT_WAV
        FILE *m_fd;
#endif
    }
    m_channels[4];

    struct sfx const &get_sfx(int n) const;

    lol::Timer m_timer;
    uint32_t m_seed;
    int m_instructions;
};

// Clamp a double to the nearest value that can be represented as a 16:16
// fixed point number (the ones used in PICO-8).
static inline int32_t double2fixed(double x)
{
    // This is more or less necessary because we use standard Lua with no
    // modifications, so we need a way to compute 1/0 or 0/0.
    if (std::isnan(x) || std::isinf(x))
        return x < 0 ? (int32_t)0x80000000 : (int32_t)0x7fffffff;
    return (int32_t)(int64_t)lol::round(x * double(1 << 16));
}

static inline double fixed2double(int32_t x)
{
    return x / double(1 << 16);
}

static inline double clamp64(double x)
{
    return fixed2double(double2fixed(x));
}

// Not a Lua function, but behaves like one
static inline double lua_toclamp64(lua_State *l, int index)
{
    return clamp64(lua_tonumber(l, index));
}

} // namespace z8

