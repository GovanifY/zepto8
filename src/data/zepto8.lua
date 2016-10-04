--
--  ZEPTO-8 — Fantasy console emulator
--
--  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
--
--  This program is free software. It comes without any warranty, to
--  the extent permitted by applicable law. You can redistribute it
--  and/or modify it under the terms of the Do What the Fuck You Want
--  to Public License, Version 2, as published by the WTFPL Task Force.
--  See http://www.wtfpl.net/ for more details.
--


--
-- Aliases for PICO-8 compatibility
--
do
    -- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
    --  coroutine.[create|resume|status|yield]() was removed in 0.1.3 but added
    --  in 0.1.6 as coroutine(), cocreate(), coresume(), costatus() and yield()
    --  respectively.
    cocreate = coroutine.create
    coresume = coroutine.resume
    costatus = coroutine.status
    yield = coroutine.yield

    -- use closure so that we don’t need “table” later
    local insert = table.insert
    local remove = table.remove

    count = function(a) return a ~= nil and #a or 0 end
    add = function(a, x) if a ~= nil then insert(a, x) end end
    sub = string.sub

    foreach = function(t, f)
        for k, v in ipairs(t) do f(v) end
    end

    all = function(a)
        local i, n = 0, a ~= nil and #a or 0
        return function() i = i + 1 if i <= n then return a[i] end end
    end

    del = function(t, v)
        if t ~= nil then
            for k, v2 in ipairs(t) do
                if v == v2 then remove(t, k) return end
            end
        end
    end

    -- All flip() does for now is yield so that the C++ VM gets a chance
    -- to draw something even if Lua is in an infinite loop
    flip = function()
        yield()
    end

    -- Backward compatibility for old PICO-8 versions
    mapdraw = _z8.map
end


--
-- Make all our C++ static methods global then hide _G
-- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
--  _G global table has been removed.
--
for k, v in pairs(_z8) do _G[k] = v end
_G = nil


--
-- Hide these modules, they should not be accessible
--
table = nil
debug = nil
string = nil
io = nil
coroutine = nil


--
-- Utility functions
--
_z8.loop = cocreate(function()
    local do_frame = true

    -- FIXME: load cart here
    if _init ~= nil then _init() end

    -- Execute the user functions
    while true do
        if _update60 ~= nil then
            _update_buttons()
            _update60()
            if _draw ~= nil then _draw() end
        elseif _update ~= nil then
            if do_frame then
                _update_buttons()
                _update()
                if _draw ~= nil then _draw() end
            end
            do_frame = not do_frame
        end
        yield()
    end
end)

_z8.tick = function()
    return coresume(_z8.loop)
end

