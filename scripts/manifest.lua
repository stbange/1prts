-- Distribution manifests use JSON strings, arrays, objects, integers and booleans.
return function(text)
    local at = 1
    local function space()
        at = text:match("^%s*()", at)
    end
    local function take(c)
        space()
        if text:sub(at, at) == c then
            at = at + 1
            return true
        end
    end
    local function need(c)
        assert(take(c), "manifest: expected " .. c .. " at " .. at)
    end
    local escapes =
        { ['"'] = '"', ["\\"] = "\\", ["/"] = "/", b = "\b", f = "\f", n = "\n", r = "\r", t = "\t" }
    local function string_value()
        need('"')
        local parts = {}
        while at <= #text do
            local c = text:sub(at, at)
            at = at + 1
            if c == '"' then
                return table.concat(parts)
            end
            if c == "\\" then
                local e = text:sub(at, at)
                at = at + 1
                if e == "u" then
                    local hex = text:sub(at, at + 3)
                    assert(hex:match("^%x%x%x%x$"), "bad unicode escape")
                    c = utf8.char(tonumber(hex, 16))
                    at = at + 4
                else
                    c = assert(escapes[e], "bad string escape")
                end
            else
                assert(c:byte() >= 32, "control character in manifest")
            end
            parts[#parts + 1] = c
        end
        error("unterminated manifest string")
    end
    local value
    function value()
        space()
        if take("{") then
            local t = {}
            if take("}") then
                return t
            end
            repeat
                local key = string_value()
                need(":")
                assert(t[key] == nil, "duplicate manifest key " .. key)
                t[key] = value()
            until not take(",")
            need("}")
            return t
        elseif take("[") then
            local t = {}
            if take("]") then
                return t
            end
            repeat
                t[#t + 1] = value()
            until not take(",")
            need("]")
            return t
        elseif text:sub(at, at) == '"' then
            return string_value()
        end
        local token = text:match("^([%w%.%-]+)", at) or ""
        at = at + #token
        if token == "true" then
            return true
        elseif token == "false" then
            return false
        end
        return assert(tonumber(token), "invalid manifest value at " .. at)
    end
    local result = value()
    space()
    assert(at > #text, "trailing manifest data")
    return result
end
