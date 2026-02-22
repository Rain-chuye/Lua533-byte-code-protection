import re

h_file = 'lua533_src/lua/llex.h'
c_file = 'lua533_src/lua/llex.c'

with open(h_file, 'r') as f:
    h_content = f.read()

# Match everything between "enum RESERVED {" and "/* other terminal symbols */"
match = re.search(r'enum RESERVED \{(.*?)/\* other terminal symbols \*/', h_content, re.S)
if not match:
    print("Could not find enum RESERVED in llex.h")
    exit(1)

raw_enum = match.group(1)
# Remove comments
raw_enum = re.sub(r'/\*.*?\*/', '', raw_enum)
# Split by comma
tokens_raw = raw_enum.split(',')
tokens = []
for t in tokens_raw:
    t = t.strip()
    if not t: continue
    # Handle TK_AND = FIRST_RESERVED
    name = t.split('=')[0].strip()
    tokens.append(name)

with open(c_file, 'r') as f:
    c_content = f.read()

array_match = re.search(r'const char \*const luaX_tokens \[\] = \{(.*?)\};', c_content, re.S)
raw_array = array_match.group(1)
raw_array = re.sub(r'/\*.*?\*/', '', raw_array)
array_tokens = [t.strip().strip('"') for t in raw_array.split(',') if t.strip()]

print(f"H tokens ({len(tokens)})")
print(f"C strings ({len(array_tokens)})")

for i in range(max(len(tokens), len(array_tokens))):
    h = tokens[i] if i < len(tokens) else "MISSING"
    c = array_tokens[i] if i < len(array_tokens) else "MISSING"
    print(f"{i:2}: {h:20} <-> {c}")
