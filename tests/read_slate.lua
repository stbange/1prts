-- A first-time-reader proxy glances at the dispatch slate every few seconds.
-- Reuse the same seed blocks when changing cues. A noise bot is measured separately.
local root = arg[1]
local game = assert(loadfile(root .. "/scripts/runtime.lua"))(root, "gate-six").gate_six
print("seed,result,first_mill_tick,accepted,rejected,first_rejection")
for _, first in ipairs({ 42, 1042 }) do
    for seed = first, first + 19 do
        local state, rnd = game.new(), seed
        local function roll(n)
            rnd = rnd * 48271 % 2147483647
            return 1 + rnd % n
        end
        local next_glance, mill, accepted, rejected, why = roll(120), -1, 0, 0, ""
        while state.result == "active" and state.tick < 7200 do
            if state.tick >= next_glance then
                local cue = state.advice
                local id = cue and roll(10) <= 8 and cue or game.actions[roll(#game.actions)][1]
                if game.command(state, id) then
                    accepted = accepted + 1
                    if id == "mill" and mill < 0 then
                        mill = state.tick
                    end
                else
                    rejected = rejected + 1
                    if why == "" then
                        why = state.report:gsub(",", " / ")
                    end
                end
                next_glance = state.tick + 45 + roll(140)
            end
            game.step(state)
        end
        print(table.concat({ seed, state.result, mill, accepted, rejected, why }, ","))
    end
end
