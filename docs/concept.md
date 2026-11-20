# 1stprts

The bow of the ferry *Kestrel* opens onto a concrete apron. From the depot gantry you can see the roof of *Mole*, a 22-metre crawler, pass just below the painted clearance line. One power core is aboard. The driver calls for a second run, and across the channel another skipper requests the same berth. You can queue *Mole*'s return trip from the gantry. What takes judgment is deciding where *Kestrel* should land, and whether you want to be on her when she leaves.

## Being the commander

You lead a small expedition rebuilding a chain of tidal ports and orbital supply stations. Your authority travels by radio. Your body travels on foot or aboard something a crew operates. A pilot can fly you to a site, and an engineer can keep a ship running while you are on another deck. Every minute you can spend looking at the world is a minute your crews can spend acting in it.

Three activities form the strategy loop.

1. **Find out what is happening.** Look out a window, walk a site, ask a crew for its status, or read a report on your slate. A colored lamp at the loading bay shows whether the ramp is ready. A remote berth marker tells you who last reported it clear and when.
2. **Commit people and material.** Choose a crew by sight or call sign, choose a job, then confirm a visible target or a reported location. A driver can queue a haul, a fitter can commission a building, and a pilot can hold at a marker. Each crew acknowledges, reports a blockage with a reason, and calls in completion. You can revise a queue while another crew works.
3. **Choose where to be next.** Ride the ferry to get an early view of the island approach, or stay ashore to survey the next build site. The ferry can finish its voyage either way. Radio calls have captions and enter a short log, so walking through a noisy engine room need never cost you an order.

The slate in your hand and the chart fixed to the bridge show symbols, routes and the ages of reports. Even while reading the chart, you can lift your eyes and see out the bridge windows. Raising a chart, giving an order, boarding and resolving an objective all keep the view at the commander's eyes. In a headset, that includes the natural separation of the two eyes.

## What makes it an RTS

The economy has **alloy**, **power** and **crew time**. A staffed reclamation yard produces alloy for construction, repairs and refits. Its output reaches the next port as freight. Power comes from staffed installations and enables workshops and relays. Crews are assigned by role. Moving the only fitter from a workshop to a construction site leaves the workshop queue waiting. The supply display distinguishes stock on hand, reserved stock and cargo in transit.

To expand, survey a footprint or use one reported by a scout, select a plan on the slate, and assign a delivery and a work crew. The shell rises as supplies arrive. Give the completed facility its operating crew and the new service joins your network. A yard can then lay down another hull, a berth can service two routes, or a relay can extend the range of current reports. The structure you commission has a door you can walk through.

Rivals use berths, crews and cargo too. In the first mission they compete for time at a shared quay. Later scenarios can ask you to escort a convoy or hold a relay while another company tries to claim it. A patrol assigned to escort follows its convoy and reports contacts. Pull it away to defend the relay and the convoy's next report may be the only news you get of the route. A disabled drive leaves a crewed hull you can enter, tow and repair. Taking over a relay requires a friendly watch in its control room and enough power to bring it online. Local crews handle driving and station work throughout.

## A world built at crew scale

One world unit is one metre. A roughly 1.7 m standing eye height, a 2.1 m personnel doorway, railings, numbered bulkheads and people on watch give you a ruler everywhere. Large assets earn their size through what fits inside them. A player should be able to park *Mole* on the ferry, climb above it and look back down at the same vehicle.

| Asset family | Example scale | A route the commander can walk | Who works there |
| --- | --- | --- | --- |
| Building | 65 m shore depot | Loading apron → dispatch office → gantry over the bay | Dispatcher and loading crew |
| Ground vehicle | 22 m cargo crawler | Rear hatch → load passage → driver's cab | Driver and loadmaster |
| Ship | 96 m roll-on ferry | Bow ramp → vehicle deck → stair tower → bridge | Pilot, deckhand and engineer |
| Aircraft | 44 m cargo tiltrotor | Tail ramp → cargo hold → flight deck | Pilot and cargo chief |
| Spacecraft | 85 m orbital tender | Docking collar → pressure lobby → habitat → flight deck | Flight crew and systems engineer |
| Space station | 800 m across its main truss | Arrival collar → transfer hall → lift → traffic control | Dock crews and watch officers |

Every building and craft in play has people on watch, a human-scale entrance and connected spaces to explore. When the rival ferry ties up, you can board it and meet a crew answering to another skipper. A landing ramp, airlock or docking collar connects actual walkable surfaces. From a moving bridge you see the quay slide away while the deckhand beside you can keep one hand on the rail. Ladders and lifts shorten trips between decks, and the radio lets you work on the way. Machinery occupies solid space on the plans. The spaces around it have room for people to do their jobs.

## First playable mission · Tide Window

**Win condition.** Bring two power cores from the mainland depot to the island relay. Its resident technicians must finish installing both and bring the relay online before high tide at 12:00 of simulation time. The clock starts after the briefing. If the tide turns first, the island crew reports the missed window.

**Starting position.** You stand at the depot with eight alloy, two cores, one fitting crew, *Mole* and *Kestrel*. Both vehicles have their own operating crew. The crawler can load the cores one at a time. The island has a public quay, a gravel slip and a staffed relay hall. A rival crewed ferry uses the public quay from 05:00 up to 08:00. The approach buoy broadcasts its reservation. Watch a clock and you can plan around it.

| Use of the fitting crew | What it buys | What you give up |
| --- | --- | --- |
| Spend six alloy to commission the depot's quay hoist | The crew brings the hoist cab online and loads both cores in one lift. Sail early for the public quay, or wait for it to clear. | The standard ferry ramp requires the public quay. A late arrival costs time. |
| Spend six alloy to fit an extended bow ramp to *Kestrel* | The pilot can land at the gravel slip during the rival's reservation. | *Mole* makes both loading trips, and the island haul from the slip is longer. |

You have enough alloy for one improvement. On either route *Mole* crosses aboard *Kestrel*, disembarks and delivers the cores. The island technicians handle installation. You can ride in the crawler, walk out onto the ferry's deck, go up to the bridge for the approach, or direct the whole trip from shore. If a berth is occupied the pilot holds, tells you why, and accepts a new destination compatible with the fitted ramp. A running ferry, a queued crew and the tide clock keep moving while you consult the chart.

A frugal commander can also keep all eight alloy and wait for the 08:00 public slot. With a prompt discharge, there is one minute to spare. The debrief records the relay's activation time and remaining alloy so the fastest crossing and the cheapest crossing each have a payoff.

The first playable succeeds if both investments can win, a missed tide produces a legible loss, and you can explain a delay from something you saw or a crew told you. Starting with a single-player operation makes it possible to test this command loop with one small set of interiors.

## Art and sound

From the quay, *Kestrel* reads as a blue hull, a white bridge and a broad orange bow door. Up close the door reveals a striped threshold, a deck number and a deckhand waving the crawler in. Use low-polygon 3D geometry for silhouettes, rooms, stairs, ramps and everything you can walk on. Use small paletted textures, mipmaps and directional sprites for distant crew, spray and work signals. Nearby crew have visible work poses at their stations. Optional voxels suit chunks of breakwater stone or cargo dressing. A missing voxel effect has no bearing on collision or navigation.

The change from an open quay to a vehicle deck should be audible. Wind gives way to a hollow ramp impact and machinery through a bulkhead. Radio lines start with the call sign. Lamps pair color with a shape or label. World textures can be coarse while chart text, captions and control labels stay legible at reading distance.

## Beyond the first port

| Stage | New decision | Playable proof |
| --- | --- | --- |
| Shore and sea | Buy throughput or access to another landing | Complete *Tide Window* from aboard the ferry and from shore. |
| Air | Send scarce lift capacity ahead, or keep the cargo together at sea | Board a staffed tiltrotor, walk from hold to cockpit in flight, then direct a handoff to the ground crew. |
| Orbit | Allocate a tender and a station berth while surface crews continue working | Ride the tender, cross its collar into the station, and receive a surface crew's progress report from traffic control. |

The first playtest should answer two questions. Can a new commander find out why a delivery stopped without leaving first person? Does moving from the depot to the bridge give them information worth making the trip for?
