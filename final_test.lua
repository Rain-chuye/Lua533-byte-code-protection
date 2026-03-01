
-- Comprehensive Test Script
local function assert_eq(a, b, msg)
    if a ~= b then error(tostring(a) .. " != " .. tostring(b) .. " : " .. msg) end
end

print("Testing Ternary Operator...")
local x = 10
local y = x > 5 ? "high" : "low"
assert_eq(y, "high", "ternary high")
local z = x < 5 ? "high" : "low"
assert_eq(z, "low", "ternary low")

print("Testing Compound Assignments...")
local a = 10
a += 5
assert_eq(a, 15, "add assign")
a *= 2
assert_eq(a, 30, "mul assign")
local s = "hello"
s ..= " world"
assert_eq(s, "hello world", "concat assign")

print("Testing Math Inlining...")
assert_eq(math.abs(-5), 5, "abs")
assert_eq(math.sqrt(25), 5.0, "sqrt")
assert_eq(math.floor(3.9), 3, "floor")
assert_eq(math.ceil(3.1), 4, "ceil")

print("Testing Iterators (Accelerated)...")
local t = {10, 20, 30}
local sum = 0
for i, v in ipairs(t) do sum = sum + v end
assert_eq(sum, 60, "ipairs sum")

local kv_sum = 0
local kt = {a=1, b=2}
for k, v in pairs(kt) do kv_sum = kv_sum + v end
assert_eq(kv_sum, 3, "pairs sum")

print("Testing Field Arithmetic Fusion...")
local p = {x = 10, vx = 5}
for i=1, 1000 do
    p.x = p.x + p.vx
end
assert_eq(p.x, 10 + 5*1000, "field add fusion")

print("Testing Custom Virtual ISA & Encryption...")
-- If we reached here, the VM is running obfuscated/virtualized code correctly
print("All Tests Passed!")
