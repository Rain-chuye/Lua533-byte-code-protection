# LXCLUA-NCore (Modernized Lua 5.3.3 VM)

LXCLUA-NCore 是基于 Lua 5.3.3 深度定制的高性能、高安全性增强版虚拟机。它引入了大量现代编程语言特性、语法糖以及多层次的安全保护机制。

## 核心特性

### 1. 现代运算符 (Modern Operators)

- **复合赋值**: `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, `&=`, `|=`, `~=`, `>>=`, `<<=`, `..=`
- **自增**: `i++` (后缀自增)
- **三目运算**: `cond ? expr1 : expr2`
- **空值处理**:
  - `??` (空值合并): `a ?? b` 等价于 `a == nil and b or a`
  - `?.` (可选链): `obj?.prop` 若 `obj` 为 `nil` 则返回 `nil`
- **管道操作**:
  - `|>` (前向管道): `x |> f` 等价于 `f(x)`
  - `<|` (反向管道): `f <| x` 等价于 `f(x)`
  - `|?>` (安全管道): `x |?> f` 若 `x` 为 `nil` 则返回 `nil`
- **比较运算符**:
  - `!=` (不等): 等价于 `~=`
  - `<=>` (三路比较): 返回 `-1`, `0` 或 `1`

### 2. 面向对象编程 (OOP)

支持完整的类系统：
- **定义**: `class Name [extends Base] ... end`
- **构造函数**: 使用 `function new(...) ... end` 或 `function constructor(...) ... end`
- **成员属性**: 支持 `public`, `private`, `protected`, `static` 修饰符（占位/语义支持）
- **访问器**: 支持 `get prop() ... end` 和 `set prop(v) ... end`
- **实例化**: `local obj = new ClassName(...)`
- **父类访问**: 支持 `Super` 关键字访问父类方法。

### 3. C 风格语法与增强

- **函数定义**: `int add(int a, int b) { return a + b }`
- **类型标注**: 支持 `int`, `float`, `bool`, `string`, `void`, `char`, `long` 等类型关键字（作为标识符兼容，并支持语义标注）。
- **结构体与枚举**: `struct`, `enum`, `namespace`, `using` 关键字支持。
- **内联汇编**: `ASM(...)` 块支持直接编写虚拟机指令。

### 4. 控制流扩展

- **Try-Catch**: `try ... catch(e) ... finally ... end`
- **Switch**: 支持语句和表达式形式 `switch(v) case 1: ... default then ... end`
- **Defer**: `defer statement` 在作用域结束时执行。
- **When**: `when cond then ... else ... end`
- **Continue**: 在循环中使用 `continue` 跳过当前迭代。
- **箭头函数**: `(args) => expr` 或 `(args) => { ... }`

### 5. 安全与防逆向保护

- **字节码加密**: 采用基于编译时间戳的动态滚动 XOR 加密。每次编译生成的字节码均不相同。
- **动态操作码映射**: 指令集在编译时随机重排，有效对抗通用反编译器。
- **SHA-256 校验**: 字节码内置完整性校验，防止篡改。
- **VM Build 锁定**: 字节码绑定特定 VM ID，确保只能在指定的 VM 版本上运行。
- **虚拟化 (VMProtect)**: 提供 `VMProtect` 库，支持将函数转换为自定义 `CYLUA` 指令集执行，隐藏原始逻辑。

### 6. 内置扩展模块

| 模块 | 功能 |
|---|---|
| `json` | 内置 JSON 序列化与反序列化 |
| `sha256` | SHA-256 哈希计算 |
| `fs` | 文件系统操作 (ls, mkdir, rm, stat等) |
| `process` | 进程管理 (getpid等) |
| `vmprotect` | 代码虚拟化保护接口 |

## 性能优化

- **Computed Gotos**: VM 分发循环采用汇编级跳转表优化，大幅提升指令执行效率。
- **编译器优化**: 默认启用 `-O3` 与 `-fomit-frame-pointer` 编译选项。

## 兼容性说明

LXCLUA-NCore 尽力保持与标准 Lua 5.3 的兼容性。新增的类型关键字（如 `string`, `int`）已通过解析器逻辑优化，不会影响对标准库（如 `string.format`）的访问。

## 构建说明

在 Linux 环境下，进入 `lua533_src/lua` 目录并运行：
```bash
make linux
```
即可生成 `lua` 解释器与 `luac` 编译器。
