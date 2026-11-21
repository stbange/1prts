-- Editable campaign field guide. The first entry of each row is a stable ID.
local catalog = { factions = {}, units = {}, buildings = {}, missions = {}, upgrades = {} }

local function entries(target, owner, rows)
    for _, row in ipairs(rows) do
        target[#target + 1] = { id = row[1], name = row[2], role = row[3], owner = owner }
    end
end

entries(catalog.factions, "basin", {
    {"weir","Weir Assembly","Terrace towns with powered yards and heavy causeway armor."},
    {"sounding","Sounding Compact","Floating towns served by amphibious carriers and mobile repair yards."},
    {"terrace","Terrace Railway","Rail settlements move crews and equipment between fixed sidings."},
    {"salt","Salt Survey","Field surveyors reopen isolated wells and chart shifting shallows."},
    {"lantern","Lantern Pilots","Air cooperatives trade flight time for fast delivery and observation."},
    {"meridian","Meridian Dock Trust","Orbital dock workers operate tugs, pressure berths and transfer depots."}
})
entries(catalog.units, "shared", {
    {"fitters","Fitting Crew","Construct facilities on surveyed pads."},
    {"mole","Mole","Tracked hauler carrying plate from a finite yard."},
    {"kestrel","Kestrel","Roll-on ferry with a walkable vehicle deck."},
    {"porter","Porter","Carries reserve operators between workplaces."},
    {"sump","Sump","Clears a flooded building entrance."},
    {"kite","Kite","Cargo tiltrotor serving the basin flight sheds."},
    {"lark","Lark","An orbital tender with a pressure lobby and crew deck."},
    {"tag","Tag Crew","Marks salvage lots and records their remaining loads."},
    {"dray","Dray","Low-bed carrier for a prefabricated building section."}
})
entries(catalog.units, "weir", {
    {"gate-section","Gate Section","Line infantry able to occupy a switch room."},
    {"lintel","Lintel","Crewed causeway tank that can contest the gate yard."},
    {"plumb","Plumb","Survey car that refreshes reports while at a benchmark."},
    {"span","Span","Engineering carrier that opens a marked crossing."},
    {"tally","Tally","Mobile support battery assigned to a reported yard."}
})
entries(catalog.units, "sounding", {
    {"channel-section","Channel Section","Infantry transported through shallow channels."},
    {"wake","Wake","Amphibious armored carrier with a troop bay."},
    {"reed","Reed","Scout launch that reports on navigable channels."},
    {"hinge","Hinge","Tow tractor for disabled hulls and floating facilities."},
    {"crossbill","Crossbill","Patrol aircraft with a manned observation cabin."}
})
entries(catalog.units, "terrace", {
    {"points","Points Crew","Sets switches and occupies rail offices."},
    {"shunter","Shunter","Moves workshop wagons between sidings."},
    {"anvil","Anvil","Armored railcar that holds an active siding."},
    {"sleeper","Sleeper","Lays replacement panels on broken track."},
    {"whistle","Whistle","Inspection car relaying line reports."}
})
entries(catalog.units, "salt", {
    {"chain-crew","Chain Crew","Surveys control points on exposed salt flats."},
    {"glint","Glint","Shallow-draft field laboratory."},
    {"barrow","Barrow","Hauls mineral samples to an assay office."},
    {"prism","Prism","Tall observation carrier for long survey shifts."},
    {"shoe","Shoe","Wide-track patrol vehicle for soft ground."}
})
entries(catalog.units, "lantern", {
    {"ground-watch","Ground Watch","Secures a landing office on foot."},
    {"tern","Tern","Small crewed courier aircraft."},
    {"gimbal","Gimbal","Mobile approach beacon with an operator cabin."},
    {"gantry","Gantry","Airfield recovery truck for a disabled aircraft."},
    {"rook","Rook","Long-endurance airborne observation crew."}
})
entries(catalog.units, "meridian", {
    {"collar-watch","Collar Watch","Takes over an inhabited docking office."},
    {"cleat","Cleat","Pressure tug for station modules."},
    {"spindle","Spindle","Transfers cargo through a docking collar."},
    {"rivet","Rivet","Crewed repair launch for station exteriors."},
    {"loom","Loom","Habitat shuttle carrying a complete shift."}
})
entries(catalog.buildings, "shared", {
    {"survey","Survey Hall","Command post and survey register."},{"dynamo","Dynamo House","Supplies six grid capacity."},
    {"mill","Plate Mill","Processes a full load into four alloy."},{"watch","Watch House","Musters infantry."},
    {"motor","Motor Hall","Produces crewed armor."},{"post","Yard Post","Houses the local defense watch."},
    {"flight","Flight Shed","Turns around patrols and cargo flights."},{"depot","Shore Depot","Stores freight close to a berth."},
    {"relay","Island Relay","Reports channel and tide status."},{"pump","Pump Room","Keeps low yard entrances dry."},
    {"stores","Plate Store","Stockpiles delivered plate."},{"infirmary","Shift Infirmary","Returns rested operators to the roster."},
    {"signals","Signal Loft","Relays reports from nearby crews."},{"slipway","Slipway","Floats completed shallow-draft hulls."},
    {"quarters","Crew Quarters","Houses workers between shifts."},{"siding","Freight Siding","Transfers freight to rail."},
    {"customs","Transit Office","Assigns shared transfer slots."},{"beacon","Beacon House","Guides a flight to a landing."},
    {"lock","Pressure Lock","Connects surface and sealed berth."},{"mooring","Tug Mooring","Attaches crewed workboats."},
    {"spaceport","Transfer Spaceport","Services orbital tender flights."}
})
entries(catalog.buildings,"weir",{{"gallery","Accumulator Gallery","Keeps one priority bay operating through a shortage."}})
entries(catalog.buildings,"sounding",{{"shed","Slip Shed","A floating repair works that can change moorings."}})
entries(catalog.buildings,"terrace",{{"roundhouse","Roundhouse","Refits rail vehicles on parallel roads."}})
entries(catalog.buildings,"salt",{{"assay","Assay Office","Identifies the yield of surveyed lots."}})
entries(catalog.buildings,"lantern",{{"tower","Approach Tower","Schedules simultaneous arrivals."}})
entries(catalog.buildings,"meridian",{{"cradle","Pressure Cradle","Services occupied orbital modules."}})
entries(catalog.buildings,"objective",{
    {"gate","Gate Six Station","Floodgate switch room overlooking the contested yard."},
    {"exchange","West Exchange","Rail junction controlling winter freight."},
    {"station","Crown Station","Orbital habitat with docking control on its main truss."}
})
entries(catalog.missions,"campaign",{
    {"gate-six","Gate Six","Build a field base and hold the floodgate for 60 seconds."},
    {"tide-window","Tide Window","Land two power cores before the public quay closes."},
    {"westbound","Westbound","Clear a freight path through the occupied West Exchange."},
    {"white-mile","White Mile","Reopen a flooded survey camp and return its last assay load."},
    {"night-mail","Night Mail","Allocate air crews to deliver the last three shift bags."},
    {"high-water","High Water","Deliver a pressure module and restore the watch on Crown Station."}
})
entries(catalog.upgrades,"campaign",{
    {"hoist","Quay Hoist","Loads two cores in one lift."},{"ramp","Extended Bow Ramp","Permits landing at the gravel slip."},
    {"points-heater","Points Heater","Keeps a junction open during the cold shift."},
    {"wide-shoes","Wide Shoes","Allows a loaded hauler onto marked salt flats."},
    {"night-board","Night Board","Adds a night arrival slot to the Flight Shed."},
    {"second-collar","Second Collar","Permits a transfer while a service tug is moored."}
})
-- Gate Six theater roster. The ship and air detachments arrive in later operations.
catalog.theater = {
    units = {"fitters","mole","porter","sump","tag","dray","gate-section","lintel","plumb","span","tally",
             "channel-section","wake","reed","hinge","points","barrow","gimbal"},
    buildings = {"survey","dynamo","mill","watch","motor","post","depot","pump","stores","infirmary","signals",
                 "slipway","quarters","siding","mooring","gallery","shed","gate"}
}
return catalog
