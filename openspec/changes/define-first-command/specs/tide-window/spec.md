# Tide Window

## Purpose

Follow Gate Six with an Assembly supply crossing, a Compact ferry using the public berth schedule and a visible tide deadline.

## ADDED Requirements

### Requirement: Make a binding investment

The mission SHALL begin with eight alloy, two power cores, one fitting crew, a staffed crawler and a staffed ferry. Commissioning the hoist or fitting the extended ramp SHALL each cost six alloy. Confirming a job SHALL reserve its stock until work starts or the queued job is canceled.

#### Scenario: Competing jobs use the same stock

- **WHEN** the commander commits six alloy to the hoist and then orders the ramp
- **THEN** the ramp reports insufficient available alloy, and starting the hoist leaves exactly two alloy and one hoist job.

#### Scenario: Cancel before work begins

- **WHEN** the commander cancels the queued six-alloy job before the fitting crew starts
- **THEN** all eight alloy become available and the other improvement can be ordered.

### Requirement: Respect the rival berth and ramp limits

The public quay SHALL be occupied by the rival during the half-open interval from 05:00 to 08:00. A landing SHALL take a continuous 30-second slot. A pilot facing an unavailable berth SHALL hold and report the reason. The gravel slip SHALL be usable by a ferry fitted with the extended ramp.

#### Scenario: Arrive too close to the reservation

- **WHEN** the standard ferry arrives at 04:45
- **THEN** its pilot holds outside the quay, reports the rival reservation, identifies the standard ramp as unsuitable for the slip, and can begin discharge at 08:00.

#### Scenario: Use the alternate landing

- **WHEN** the extended-ramp ferry reaches the slip at 05:20 while the rival uses the quay
- **THEN** both ferries can use their separate landings and the crawler can unload at the slip.

### Requirement: Resolve the mission from delivered cargo and installed power

The game SHALL award victory when the island technicians have installed both delivered cores and the relay comes online before 12:00 of simulation time. Reaching 12:00 with the objective incomplete SHALL end the mission in a loss. The commander's location SHALL have no bearing on the outcome.

#### Scenario: Win with the hoist

- **WHEN** the hoist loads both cores, the pilot reaches the quay before the rival, the crawler delivers them and technicians complete installation before high tide
- **THEN** the relay reports online, both cores are accounted for at the relay and the commander receives victory, including when directing from shore.

#### Scenario: Win with the extended ramp

- **WHEN** the crawler loads both cores, the refitted ferry uses the slip and the technicians finish before high tide
- **THEN** the same victory condition is met, including when the commander rides aboard.

#### Scenario: Save the alloy

- **WHEN** the commander ships both cores with the standard equipment, takes the 08:00 quay slot and completes the remaining work without delay
- **THEN** the debrief records an 11:00 relay activation and eight alloy remaining.

#### Scenario: Reach the exact deadline

- **WHEN** the second installation would first complete at 12:00 or later
- **THEN** high tide ends the mission in a loss and no later event changes that result.
