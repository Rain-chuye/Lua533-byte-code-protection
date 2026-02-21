local A = { a = 1 }
local B = setmetatable({ b = 2 }, { __index = A })
print(B.a)
