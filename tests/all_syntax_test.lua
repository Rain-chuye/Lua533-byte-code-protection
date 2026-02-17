-- Comprehensive Syntax Test for Lua 5.3 + Custom Opcodes

print("--- Start Comprehensive Syntax Test ---")

-- 1. Basic Arithmetic and Logic
local a = 10 + 20 - 5 * 2 / 1 % 3 ^ 2 // 1
local b = (10 > 5) and (5 < 10) or not (10 == 10)
print("Arithmetic & Logic:", a, b)

-- 2. Bitwise Operators
local c = (0x0F & 0x55) | (0xAA ~ 0xFF) << 2 >> 1
print("Bitwise:", string.format("0x%X", c))

-- 3. Strings and Concatenation
local s = "Hello" .. " " .. "World"
print("String:", s, "Length:", #s)

-- 4. Tables and Constructors
local t = { 1, 2, 3; x = 10, ["y"] = 20 }
t[2] = 42
print("Table:", t[1], t[2], t.x, t.y)

-- 5. Loops
print("Loops:")
local sum = 0
for i = 1, 5 do sum = sum + i end
print("  For:", sum)

local j = 1
while j <= 5 do sum = sum - j; j = j + 1 end
print("  While:", sum)

repeat sum = sum + 1 until sum >= 5
print("  Repeat:", sum)

-- 6. Functions, Closures and Upvalues
local function outer(x)
    return function(y)
        return x + y
    end
end
local inner = outer(10)
print("Closure:", inner(5))

-- 7. Varargs
local function vararg_test(...)
    local args = {...}
    return #args
end
print("Vararg:", vararg_test(1, 2, 3, 4))

-- 8. Coroutines
local co = coroutine.create(function(x)
    coroutine.yield(x + 1)
    return x + 2
end)
local _, r1 = coroutine.resume(co, 10)
local _, r2 = coroutine.resume(co)
print("Coroutine:", r1, r2)

-- 9. Metatables
local mt = {
    __add = function(op1, op2)
        return op1.val + op2.val
    end
}
local o1 = { val = 10 }
local o2 = { val = 20 }
setmetatable(o1, mt)
print("Metatable __add:", o1 + o2)

-- 10. Ternary Operator (Custom)
local x = 10
local y = (x > 5) ? "Greater" : "Smaller"
print("Ternary:", y)

-- 11. Fusion Opcodes Tests
local function test_fusion()
    local ft = { x = 100, y = 200 }
    -- ft.x = ft.x + 10 -- Try to match FUSE_ADD_TO_FIELD
    ft.x = ft.x + 10
    -- local sub = ft.x - 5 -- Try to match FUSE_GETSUB
    local sub = ft.x - 5
    -- local add = ft.x + 5 -- Try to match FUSE_GETADD
    local add = ft.x + 5
    -- dx*dx + dy*dy ^ 0.5 -- FAST_DIST
    local dx = 3
    local dy = 4
    local dist = (dx * dx + dy * dy) ^ 0.5
    print("Fusion Opcodes:", ft.x, sub, add, dist)
end
test_fusion()

-- 12. NEWARRAY (Custom)
local arr = [10, 20, 30]
print("NewArray:", arr[1], arr[2], arr[3])

-- 13. Error Handling
local status, err = pcall(function()
    error("手动错误")
end)
print("Pcall Status:", status, "Error:", err)

print("--- Comprehensive Syntax Test Complete ---")
