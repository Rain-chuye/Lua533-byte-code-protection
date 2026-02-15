local luajava = { bindClass = function() return { VERSION = { SDK_INT = 24 } } end }
local android = { R = { style = { Theme_Material = "M", Theme_Material_Light = "ML" } } }
local os = { date = function() return "12" end }
local activity = { setTheme = print }

local version = 24;
local function autotheme()
  local h=12
  if version >= 21 then
    if h<=6 or h>=22 then
      return "M"
    else
      return "ML"
    end
  else
    return "H"
  end
end
activity.setTheme(autotheme())
