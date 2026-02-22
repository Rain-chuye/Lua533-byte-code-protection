# LXCLUA-NCore Lua 5.3.3 VM (Hardened & Extended)

This is a heavily customized Lua 5.3.3 Virtual Machine with modern syntax extensions, enhanced security, and built-in library modules.

## New Syntax & Modern Features

### 1. Compound Assignment Operators
Supports: `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, `&=`, `|=`, `~=`, `>>=`, `<<=`, `..=`
```lua
local x = 10
x += 5
print(x) -- 15
```

### 2. Modern Operators
- **Ternary**: `condition ? expr1 : expr2`
- **Null Coalescing**: `expr ?? fallback`
- **Optional Chaining**: `obj?.property`
- **Spaceship**: `a <=> b` (returns -1, 0, or 1)
- **Pipelines**: `x |> f` (forward), `f <| x` (backward), `x |?> f` (safe)

### 3. Functional Constructs
- **Arrow Functions**: `(x, y) => x + y` or `(x) => { return x * 2 }`
- **Lambda**: `\x -> x + 1`

### 4. Object Oriented Programming (OOP)
Built-in `class` keyword with single inheritance.
```lua
class Base
  function init(self, name) self.name = name end
end

class Derived extends Base
  function hello(self) print("Hello, " .. self.name) end
end

local obj = new Derived("World")
obj:hello()
```

### 5. Control Flow Extensions
- **Switch-Case**:
```lua
switch(val) do
  case 1: print("one")
  case 2, 3: print("two/three")
  default: print("default")
end
```
- **When-Case**: Alternative `if-elseif` syntax.
- **Defer**: `defer print("done") end`
- **Continue**: `continue` in loops.
- **Try-Catch**:
```lua
try
  error("fail")
catch (e)
  print("Caught:", e)
finally
  print("Always runs")
end
```

### 6. Asynchronous Programming
- **Async Functions**: `async function foo() ... end`
- **Await**: `val = await task`

## Built-in Modules
The VM includes pre-linked modules:
- `json`: `encode`, `decode`
- `sha256`: `hash`
- `fs`: `ls`, `mkdir`, `rm`, `stat`, `exists`
- `http`: `get`, `post`
- `thread`: `create`, `send`, `receive`
- `process`: `fork`, `wait`, `readmem` (Linux only)
- `struct`: `pack`, `unpack`
- `ptr`: Pointer arithmetic and memory access.

## Security & Obfuscation
- **Custom CYLUA ISA**: Randomized opcode mapping and randomized instruction bit-fields (ABC/ACB/BAC...).
- **Polymorphic Encryption**: Instructions are encrypted with per-function seeds.
- **Constant Hardening**: Strings and integers are decrypted on-the-fly into temporary buffers.
- **Anti-Debugging**: Stripped debug info and internal security checks.

## Building
```bash
cd lua
make linux
```

## Running
```bash
./lua/lua your_script.lua
```
