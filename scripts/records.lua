-- Deterministic data records keep numeric table keys distinct from strings.
-- Tables are encoded as ordered key/value pairs. No saved text is executed.
local M = {}
local function quote(s)
    return '"'
        .. s:gsub('[%z\1-\31\\"]', function(c)
            if c == '"' or c == "\\" then
                return "\\" .. c
            end
            return ("\\u%04x"):format(c:byte())
        end)
        .. '"'
end
local function encode(value, seen)
    local kind = type(value)
    if kind == "string" then
        return quote(value)
    end
    if kind == "boolean" then
        return tostring(value)
    end
    if kind == "number" then
        assert(value == value and math.abs(value) < math.huge, "non-finite record value")
        return math.type(value) == "integer" and tostring(value) or ("%.17g"):format(value)
    end
    assert(kind == "table" and not seen[value], "cyclic or unsupported record")
    seen[value] = true
    local keys, fields = {}, {}
    for key in pairs(value) do
        assert(type(key) == "number" or type(key) == "string", "invalid record key")
        keys[#keys + 1] = key
    end
    table.sort(keys, function(a, b)
        if type(a) == type(b) then
            return a < b
        end
        return type(a) == "number"
    end)
    for _, key in ipairs(keys) do
        fields[#fields + 1] = encode(key, seen)
        fields[#fields + 1] = encode(value[key], seen)
    end
    seen[value] = nil
    return "[" .. table.concat(fields, ",") .. "]"
end
function M.encode(value)
    return encode(value, {})
end
function M.decode(text, parse)
    local function unpack_value(value)
        if type(value) ~= "table" then
            return value
        end
        assert(#value % 2 == 0, "incomplete saved record")
        local result = {}
        for i = 1, #value, 2 do
            local key = value[i]
            assert(
                (type(key) == "string" or type(key) == "number") and result[key] == nil,
                "invalid saved key"
            )
            result[key] = unpack_value(value[i + 1])
        end
        return result
    end
    return unpack_value(parse(text))
end
return M
