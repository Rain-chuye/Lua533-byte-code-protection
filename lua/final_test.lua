
-- Test 1: Simple constants (OP_TERNARY)
print(true ? 1 : 2)
print(false ? 1 : 2)

-- Test 2: Strings and Booleans
print(true ? "yes" : "no")
print(false ? "yes" : "no")
print(true ? true : false)

-- Test 3: Short-circuiting (Jumps)
print(true ? "ok" : error("fail"))
print(false ? error("fail") : "ok")

-- Test 4: Nested Ternary
print(true ? (false ? "a" : "b") : "c")
print(false ? "a" : (true ? "b" : "c"))

-- Test 5: Mixed types and variables
local x = 10
local y = 20
print(x < y ? x : y)
print(x > y ? x : y)

-- Test 6: Method calls and ambiguity
local t = {
  val = 42,
  method = function(self) return self.val end
}
print(true ? t:method() : 0)
print(false ? 0 : t:method())

-- Test 7: Error messages (Chinese)
print("Testing error messages...")
local status, err = pcall(function() return 1 + "a" end)
print(err)
status, err = pcall(function() local a = nil; return a.x end)
print(err)
status, err = pcall(function() return {} > 1 end)
print(err)
