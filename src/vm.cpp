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

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

vm::vm()
  : m_instructions(0)
{
    lol::LuaState *l = GetLuaState();

    // Store a pointer to us in global state
    set_this(l);

    // Automatically yield every 1000 instructions
    lua_sethook(l, &vm::hook, LUA_MASKCOUNT, 1000);

    // Register our Lua module
    lol::LuaObjectDef::Register<vm>(l);

    ExecLuaFile("data/zepto8.lua");

    // Initialise the PRNG
    ExecLuaCode("srand(0)");

    // Load font
    m_font.Load("data/font.png");

    // Allocate memory
    m_memory.resize(SIZE_MEMORY);

    // Clear screen
    ::memset(m_memory.data() + OFFSET_SCREEN, 0, SIZE_SCREEN);
}

vm::~vm()
{
}

void vm::set_this(lol::LuaState *l)
{
    lua_pushlightuserdata(l, this);
    lua_setglobal(l, "\x01");
}

vm* vm::get_this(lol::LuaState *l)
{
    lua_getglobal(l, "\x01");
    vm *ret = (vm *)lua_touserdata(l, -1);
    lua_remove(l, -1);
    return ret;
}

void vm::hook(lol::LuaState *l, lua_Debug *)
{
    vm *that = get_this(l);

    // The value 35000 was found using trial and error
    that->m_instructions += 1000;
    if (that->m_instructions >= 35000)
        lua_yield(l, 0);
}

void vm::load(char const *name)
{
    m_cart.load(name);

    // Copy everything up to the code section into memory
    ::memcpy(m_memory.data(), m_cart.get_rom().data(), OFFSET_CODE);
}

void vm::run()
{
    // Start the cartridge!
    ExecLuaCode("run()");
}

void vm::step(float seconds)
{
    UNUSED(seconds);

    luaL_dostring(GetLuaState(), "_z8.tick()");
    m_instructions = 0;
}

const lol::LuaObjectLib* vm::GetLib()
{
    static const lol::LuaObjectLib lib = lol::LuaObjectLib(
        "_z8",

        // Statics
        {
            { "run",      &vm::run },
            { "menuitem", &vm::menuitem },
            { "cartdata", &vm::cartdata },
            { "reload",   &vm::reload },
            { "peek",     &vm::peek },
            { "poke",     &vm::poke },
            { "memcpy",   &vm::memcpy },
            { "memset",   &vm::memset },
            { "dget",     &vm::dget },
            { "dset",     &vm::dset },
            { "stat",     &vm::stat },
            { "printh",   &vm::printh },

            { "_update_buttons", &vm::update_buttons },
            { "btn",  &vm::btn },
            { "btnp", &vm::btnp },

            { "cursor", &vm::cursor },
            { "print",  &vm::print },

            { "max",   &vm::max },
            { "min",   &vm::min },
            { "mid",   &vm::mid },
            { "flr",   &vm::flr },
            { "cos",   &vm::cos },
            { "sin",   &vm::sin },
            { "atan2", &vm::atan2 },
            { "sqrt",  &vm::sqrt },
            { "abs",   &vm::abs },
            { "sgn",   &vm::sgn },
            { "rnd",   &vm::rnd },
            { "srand", &vm::srand },
            { "band",  &vm::band },
            { "bor",   &vm::bor },
            { "bxor",  &vm::bxor },
            { "bnot",  &vm::bnot },
            { "shl",   &vm::shl },
            { "shr",   &vm::shr },

            { "camera",   &vm::camera },
            { "circ",     &vm::circ },
            { "circfill", &vm::circfill },
            { "clip",     &vm::clip },
            { "cls",      &vm::cls },
            { "color",    &vm::color },
            { "fget",     &vm::fget },
            { "fset",     &vm::fset },
            { "line",     &vm::line },
            { "map",      &vm::map },
            { "mget",     &vm::mget },
            { "mset",     &vm::mset },
            { "pal",      &vm::pal },
            { "palt",     &vm::palt },
            { "pget",     &vm::pget },
            { "pset",     &vm::pset },
            { "rect",     &vm::rect },
            { "rectfill", &vm::rectfill },
            { "sget",     &vm::sget },
            { "sset",     &vm::sset },
            { "spr",      &vm::spr },
            { "sspr",     &vm::sspr },

            { "music", &vm::music },
            { "sfx",   &vm::sfx },

            { "time", &vm::time },

            { nullptr, nullptr }
        },

        // Methods
        {
            { nullptr, nullptr },
        },

        // Variables
        {
            { nullptr, nullptr, nullptr },
        });

    return &lib;
}

vm* vm::New(lol::LuaState* l, int argc)
{
    // FIXME: I have no idea what this function is for
    UNUSED(l);
    msg::info("requesting new(%d) on vm\n", argc);
    return nullptr;
}

//
// System
//

int vm::run(lol::LuaState *l)
{
    vm *that = get_this(l);

    // Initialise VM state (TODO: check what else to init)
    ::memset(that->m_buttons, 0, sizeof(that->m_buttons));

    // From the PICO-8 documentation:
    // “The draw state is reset each time a program is run. This is equivalent to calling:
    // clip() camera() pal() color()”
    //
    // Note from Sam: this should probably be color(6) instead.
    if (luaL_loadstring(l, "clip() camera() pal() color(6)") == 0)
        lua_pcall(l, 0, LUA_MULTRET, 0);

    // FIXME: this should probably go into _z8.tick()

    // Load cartridge code into a global identifier
    if (luaL_loadstring(l, that->m_cart.get_lua().C()) == 0)
    {
        lua_pushvalue(l, -1);
        lua_setglobal(l, ".code");
    }

    // Execute cartridge code
    lua_getglobal(l, ".code");
    lua_pcall(l, 0, LUA_MULTRET, 0);

    return 0;
}

int vm::menuitem(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:menuitem\n");
    return 0;
}

int vm::cartdata(lol::LuaState *l)
{
    int x = (int)lua_tonumber(l, 1);
    msg::info("z8:stub:cartdata %d\n", x);
    return 0;
}

int vm::reload(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:reload\n");
    return 0;
}

int vm::peek(lol::LuaState *l)
{
    int addr = (int)lua_tonumber(l, 1);
    if (addr < 0 || addr >= SIZE_MEMORY)
        return 0;

    vm *that = get_this(l);
    lua_pushnumber(l, that->m_memory[addr]);
    return 1;
}

int vm::poke(lol::LuaState *l)
{
    int addr = (int)lua_tonumber(l, 1);
    int val = (int)lua_tonumber(l, 2);
    if (addr < 0 || addr >= SIZE_MEMORY)
        return 0;

    vm *that = get_this(l);
    that->m_memory[addr] = (uint16_t)val;
    return 0;
}

int vm::memcpy(lol::LuaState *l)
{
    int dst = lua_tonumber(l, 1);
    int src = lua_tonumber(l, 2);
    int size = lua_tonumber(l, 3);

    /* FIXME: should the memory wrap around maybe? */
    if (dst >= 0 && src >= 0 && size > 0
         && dst < SIZE_MEMORY && src < SIZE_MEMORY
         && src + size <= SIZE_MEMORY && dst + size <= SIZE_MEMORY)
    {
        vm *that = get_this(l);
        memmove(that->m_memory.data() + dst, that->m_memory.data() + src, size);
    }

    return 0;
}

int vm::memset(lol::LuaState *l)
{
    int dst = lua_tonumber(l, 1);
    int val = lua_tonumber(l, 2);
    int size = lua_tonumber(l, 3);

    if (dst >= 0 && size > 0
         && dst < SIZE_MEMORY && dst + size <= SIZE_MEMORY)
    {
        vm *that = get_this(l);
        ::memset(that->m_memory.data() + dst, val, size);
    }

    return 0;
}

int vm::dget(lol::LuaState *l)
{
    msg::info("z8:stub:dget\n");
    lua_pushnumber(l, 0);
    return 1;
}

int vm::dset(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:dset\n");
    return 0;
}

int vm::stat(lol::LuaState *l)
{
    msg::info("z8:stub:stat\n");
    lua_pushnumber(l, 0);
    return 1;
}

int vm::printh(lol::LuaState *l)
{
    char const *str;
    if (lua_isnoneornil(l, 1))
        str = "false";
    else if (lua_isstring(l, 1))
        str = lua_tostring(l, 1);
    else
        str = lua_toboolean(l, 1) ? "true" : "false";

    fprintf(stdout, "%s\n", str);
    fflush(stdout);

    return 0;
}

//
// I/O
//

int vm::update_buttons(lol::LuaState *l)
{
    vm *that = get_this(l);

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (that->m_buttons[1][i])
            ++that->m_buttons[0][i];
        else
            that->m_buttons[0][i] = 0;
    }

    return 0;
}

int vm::btn(lol::LuaState *l)
{
    vm *that = get_this(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= that->m_buttons[0][i] ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, that->m_buttons[0][index]);
    }

    return 1;
}

int vm::btnp(lol::LuaState *l)
{
    auto was_pressed = [](int i)
    {
        // “Same as btn() but only true when the button was not pressed the last frame”
        if (i == 1)
            return true;
        // “btnp() also returns true every 4 frames after the button is held for 15 frames.”
        if (i > 15 && i % 4 == 0)
            return true;
        return false;
    };

    vm *that = get_this(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= was_pressed(that->m_buttons[0][i]) ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, was_pressed(that->m_buttons[0][index]));
    }

    return 1;
}

//
// Sound
//

int vm::music(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:music\n");
    return 0;
}

int vm::sfx(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:sfx\n");
    return 0;
}

//
// Deprecated
//

int vm::time(lol::LuaState *l)
{
    vm *that = get_this(l);
    float time = lol::fmod(that->m_timer.Poll(), 65536.0f);
    lua_pushnumber(l, time < 32768.f ? time : time - 65536.0f);
    return 1;
}

} // namespace z8

