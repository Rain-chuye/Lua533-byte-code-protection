function autotheme()
  return "success"
end

local function outer()
  local count = 0
  while count < 5 do
    local res = autotheme()
    if res ~= "success" then
      error("fail")
    end
    count = count + 1
  end
  return true
end

print("Running complex test...")
if outer() then
  print("Complex test passed!")
end
