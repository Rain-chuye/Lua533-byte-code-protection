function foo() return "foo" end
function bar() return "bar" end

local x = 10
local y = (x > 5) ? foo() : bar()
print("y:", y)
assert(y == "foo")

local z = (x < 5) ? foo() : bar()
print("z:", z)
assert(z == "bar")
