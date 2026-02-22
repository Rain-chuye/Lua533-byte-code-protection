local json = require('json'); local t = json.decode('{"a": 1, "b": "test"}'); print(t.a); print(t.b)
