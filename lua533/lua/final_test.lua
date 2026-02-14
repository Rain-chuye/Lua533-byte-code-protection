function fact(n) if n==0 then return 1 else return n*fact(n-1) end end; local res = fact(5); if res == 120 then print('PASS') else print('FAIL') end
