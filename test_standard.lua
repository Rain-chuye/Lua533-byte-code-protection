function autotheme(x)
  print("autotheme called:", x)
  return x
end

local a = autotheme(123)
print("a:", a)
assert(a == 123)
