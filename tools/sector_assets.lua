-- Rebuild the browser sector inventory with lua tools/sector_assets.lua build/bin > assets/sectors.txt.
local root = assert(arg[1], "supply the configured content directory")
local runtime = assert(loadfile(root .. "/scripts/runtime.lua"))
for _, mission in ipairs(runtime(root, "gate-six").operations) do
    local scene = runtime(root, mission.id)
    local meshes = { ["operator-side"] = true, ["operator-work"] = true }
    for _, row in ipairs(scene.site) do
        meshes[row[1]] = true
    end
    local paths = {}
    for mesh in pairs(meshes) do
        paths[#paths + 1] = mesh
    end
    table.sort(paths)
    print(mission.id .. " " .. table.concat(paths, " "))
end
