# World targeting

## ADDED Requirements

### Requirement: Address targets in every sector from the commander's body

Desktop, touch and WebXR SHALL resolve the same eligible crew, entity, pad, berth, reported contact and terrain target through a common order path. Direct world pointing SHALL use the first-person pose, visible surfaces and the current moving frames. The chart SHALL support reported targets with a source and observation age.

#### Scenario: Redirect a ferry

- **WHEN** the commander selects the ferry pilot and confirms the eligible alternate landing from the desktop view, touch view or XR controller ray
- **THEN** each input produces one order to the same pilot and landing, the crew acknowledges, and the route changes while the commander's view stays aboard.

#### Scenario: Point beyond Gate Six

- **WHEN** an XR commander selects a reachable rail switch in Westbound or a docking collar in High Water
- **THEN** the indicated object highlights and confirmation dispatches the selected job to that object's instance.

### Requirement: Keep confirmation and cancellation reliable in motion

The command interface SHALL preview the job, recipient and target and revalidate them on confirmation. A canceled draft SHALL spend no resources or issue an order. Both eyes SHALL present the same selection, and a change of active input SHALL preserve the pending draft and commander pose.

#### Scenario: The chosen target becomes unavailable

- **WHEN** a target moves behind an occluder before initial picking or a previously selected berth becomes unavailable before confirmation
- **THEN** picking respects the near surface, the unavailable berth gives a readable reason, and the player can choose another target.

#### Scenario: Keep a draft while changing views

- **WHEN** a player rotates the touch display or exits XR aboard a moving vehicle with an order drafted
- **THEN** the crew, verb and destination survive the transition and one subsequent confirmation produces one order.
