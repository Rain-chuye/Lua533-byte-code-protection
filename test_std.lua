local t = { a = 1 }
function t:foo() return self.a end
print(t:foo())
