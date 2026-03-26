# Alpha Final Implementation Spec

## 1. Purpose

This document is the final engineering handoff spec for **Alpha**.

It consolidates the current decision backlog and earlier supporting specs into one implementation-ready reference.

### Source precedence

Use this precedence order when documents conflict:
1. **Alpha decision backlog** is the main source of truth.
2. Supporting rules specs fill in implementation detail where they do not conflict.
3. Older foundation language is descriptive only and does not override locked decisions.

### One required corrective resolution

There is one internal contradiction in the current materials:
- the decision backlog says Alpha building scope is limited to **Warehouse I** and **Estate I** with exactly **2 internal building slots**
- other materials still include **tools, goods, tool workshop, and goods workshop** as active Alpha systems
- the backlog also makes **founding cost require 5 tools**

These cannot all be true at once.

**Resolution for this handoff:**
- Alpha shipping implementation uses a **3-resource active economy**: **food, wood, stone**
- **tools** and **goods** remain reserved in the data schema for future expansion, but are **disabled in gameplay**
- **Tool Workshop**, **Goods Workshop**, **Mill**, and **Granary** are **not implemented in Alpha**
- founding cost is corrected from **5 tools** to **0 tools** so later settlement founding remains possible

This is the smallest correction that preserves the locked building scope, fixed slot model, and overall Alpha design.

---

## 2. Alpha scope

Alpha is a **single-society feudal agrarian sandbox**.

### Hard exclusions
Do not build:
- computer players
- warfare
- diplomacy
- politics
- multiple governments
- religion systems
- long-distance trade systems
- broad technology trees
- manual house placement
- individual worker path simulation
- soldiers
- housing simulation
- overcrowding
- manual urban expansion direction
- instant resolve or step-through mode

### Alpha must prove
- terrain matters
- settlement placement matters
- labor range matters
- roads matter
- local stockpiles matter
- zoning matters
- farmland expansion and retreat matter
- settlement growth changes land use
- logistics creates local surpluses, shortages, and bottlenecks

---

## 3. Core simulation model

### Time model
- One simulation pass equals **one month**.
- The game is **strictly turn-advanced by player input**.
- No hidden simulation runs between turns.
- Rendering is continuous, but world state advances only on monthly turns.
- Simulation must be deterministic for the same seed and same inputs.

### Visible monthly phases
Show the month as **5 visible UI phases**:
1. Season & Labor
2. Production
3. Logistics
4. Consumption
5. Construction & Settlement Update

### Exact internal monthly order
1. Apply month and season modifiers and recalculate labor demand.
2. Allocate labor.
3. Produce resources.
4. Resolve logistics and stockpile transfers.
5. Consume food and apply shortage effects.
6. Advance construction, founding, and settlement expansion.
7. Commit final month-end state.

### Presentation and performance targets
- Minimal animation only.
- No sprite-based presentation.
- Visible monthly resolution target: about **1.5 seconds**.
- Target compute budget:
  - under **300 ms** monthly sim at normal load
  - under **750 ms** monthly sim at heavy late-game load
- Target world scale:
  - 1024 x 1024 cells
  - up to 48 settlements
  - up to 400 zones
  - up to 40,000 road cells
  - up to 200 active projects

### Numeric storage
- Store resource values internally as **integer tenths**.
- Display resources to **one decimal place**.
- Population may use hidden fractional accumulators for growth and starvation.

---

## 4. Map and terrain

### Map format
- Fixed map size: **1024 x 1024**.
- Fine raster grid, not hexes.
- One cell is gameplay-small, roughly one road-width.
- Mental scale can be treated as approximately **20 meters per cell**, but this is abstract and gameplay-first.

### Per-cell state
Each cell stores only:
- elevation: 0-255
- slope: 0-4
- water: 0-3
- fertility: 0-100
- vegetation: 0-100

Do not add more cell state in Alpha.

### Water tiers
- 0 = dry land
- 1 = wet / shallow / fordable ground
- 2 = deep inland / coastal water
- 3 = major water / ocean

### Slope tiers
- 0 = flat
- 1 = gentle
- 2 = rolling
- 3 = steep
- 4 = cliff / impassable

### Land traversability
- Dry land with slope 0-3 is traversable.
- Water tier 1 with slope 0-3 is traversable at high cost.
- Slope 4 is impassable.
- Water tier 2-3 is impassable to land movement.

### Non-road movement cost per cell
- base 1.0
- slope penalty:
  - slope 0: +0.0
  - slope 1: +0.25
  - slope 2: +0.75
  - slope 3: +1.5
  - slope 4: impassable
- vegetation penalty:
  - 0-24: +0.0
  - 25-49: +0.15
  - 50-74: +0.35
  - 75-100: +0.7
- water penalty:
  - water 0: +0.0
  - water 1: +1.0
  - water 2-3: impassable

### Road legality
Roads may be built on:
- dry land with slope 0-3
- water tier 1 with slope 0-2

Roads may not be built on:
- slope 4
- water tier 2-3

### Road movement cost per cell
- base 0.35
- slope road penalty:
  - slope 0: +0.0
  - slope 1: +0.05
  - slope 2: +0.15
  - slope 3: +0.35
  - slope 4: impassable
- water road penalty:
  - water 0: +0.0
  - water 1: +0.25
  - water 2-3: impassable

### Map generation
- Fully procedural.
- No settlement suitability score.
- Procedural continent, inland water, and biome targets are implementation-owned.
- Output must prioritize readability and stable terrain bands over noisy visual detail.

---

## 5. Settlements

### Settlement type
- Only one settlement type exists in Alpha.
- Settlements differ by population size, footprint, and current internal building state.

### Settlement data model
Each settlement must track at minimum:
- settlement id
- center point
- owned footprint cells
- population whole count
- hidden population fractional accumulator
- current development pressure
- local stockpile
- current labor assignment results
- owned zone components
- active projects
- founding source settlement id if this settlement is still being founded
- connectivity and cached access data as needed

### Important cut
Do **not** track:
- housing capacity
- overcrowding
- persistent social classes
- manual internal building placement

### Starting settlement and later founded settlements
Use the same template for both.

#### Settlement start state
- population: 20 generic pops
- stockpile:
  - 40 food
  - 20 wood
  - 10 stone
- buildings:
  - Estate I
  - Warehouse I

### Founding new settlements
Founding is a project created by an existing source settlement.

#### Founding rules
- player selects a source settlement
- player selects a target point
- founding transfers population and starting resources from the source settlement
- founding takes multiple months
- on completion, the new settlement node appears and begins using normal monthly simulation

#### Founding site validity
A founding target is legal only if:
- target center cell has water tier 0
- target center cell has slope 0-2
- target is reachable from source settlement by a traversable land path
- initial 5 x 5 founding footprint does not overlap another settlement footprint
- initial 5 x 5 founding footprint does not overlap zoned cells
- within the initial 5 x 5 footprint, at least 16 cells are dry land with slope 0-2

#### Founding cost
Because of the tools contradiction described above, use this corrected Alpha founding cost:
- 20 population
- 100 food
- 50 wood
- 25 stone
- 150 common work
- 30 skilled work

#### New settlement spawn state on completion
- 20 generic pops
- 40 food
- 20 wood
- 10 stone
- Estate I
- Warehouse I

---

## 6. Population and roles

### Population model
- Population is a single generic pool.
- Roles are staffing states, not persistent demographic classes.
- Relevant staffed roles:
  - serfs
  - artisans
  - nobles
- Role reassignment is effectively instant each monthly pass, subject to labor allocation inertia.

### Growth and mortality
Baseline monthly change:
- growth: +0.4% of current population
- mortality: -0.2% of current population
- normal net: +0.2%

Accumulate fractions internally until a whole pop is created or removed.

### Food shortage and starvation
Food is the only non-baseline factor affecting population change.

#### Food coverage
- food coverage = food actually consumed / food required
- clamp to 0.0-1.0

#### Population effect by coverage
- if coverage = 1.0: apply normal growth
- if coverage < 1.0: cancel normal growth for that month
- starvation bands:
  - 0.75 to 0.99: no starvation deaths
  - 0.50 to 0.74: -1.0% population
  - 0.25 to 0.49: -3.0% population
  - 0.00 to 0.24: -6.0% population

Starvation removes population from the generic pool at month end.

---

## 7. Labor allocation

### Core rule
Priorities bias marginal labor allocation. They do not erase the settlement’s base structure.

### Three-layer labor model
1. protected base roles
2. priority-weighted extra roles
3. inertia

### Protected base roles
Protected base roles preserve settlement identity and survival.

In Alpha, protected base roles are:
- food-floor serf staffing on the best active farmland
- required minimum staffing for active internal buildings

### Extra roles
Extra roles include:
- additional farmland staffing beyond the food floor
- field opening labor
- forestry labor
- quarry labor
- construction labor
- any optional building staffing above base minimums

### Priority labels
Use:
- Required
- High
- Normal
- Low
- Paused

Required is for protected base roles and core system essentials only.

### Hard rule for construction
Construction is **never** a protected base role.
It is always extra-role demand.
Construction should be the easiest labor to reassign away from.

### Crisis degradation order
When labor is scarce:
1. extra slots shrink first
2. construction slows or pauses
3. optional building staffing drops
4. extra noble staffing drops
5. protected non-food base roles are squeezed only in deeper crisis
6. only severe crisis threatens basic settlement identity

### Nobles
- Nobles do not exist as a large idle reserve pool.
- If noble slots are not filled, those roles remain vacant.

### Staffing pass order
1. Compute active base roles, extra-role demand, and available population.
2. Fill protected base roles.
3. Distribute remaining labor across extra roles using priority weights.
4. Apply reassignment limits and inertia.
5. Leave leftover labor idle.

### Tuning areas
Implementation-owned:
- exact food-floor serf threshold math
- extra-role weights
- reassignment fraction limits
- inertia constants

These must preserve the principles above.

---

## 8. Buildings

### Shipping Alpha building scope
Only these internal buildings exist:
- Warehouse I
- Estate I

### Building rules
- Buildings are abstractly placed inside the settlement footprint.
- No manual internal placement.
- Buildings are unique per settlement.
- Only Tier I exists.
- Max internal building slots = 2:
  - Warehouse slot
  - Estate slot

### Prerequisites
- Warehouse I: settlement exists, no existing Warehouse, construction costs can be paid
- Estate I: settlement exists, no existing Estate, construction costs can be paid

### Estate I
- optional, not mandatory
- requires at least 1 noble to function
- only bonus in Alpha:
  - if Estate I exists and has at least 1 noble staffed, all active farmland worked by that settlement gets **+25% food output**
- bonus does not scale with extra nobles

### Warehouse I
- general storage building line
- only Tier I exists in Alpha
- exact capacity values are implementation-owned tuning

---

## 9. Land-use zoning

### Zone types
- farmland
- forestry
- quarry

### Ownership model
- every external zone belongs to exactly one settlement
- player paints zones for a selected settlement
- only the owner settlement can work the zone directly
- output enters that settlement’s stockpile

### Operational reach
A cell can be zoned only if it is within the selected settlement’s operational reach.

Zoning legality is based on **cheapest-path access cost** from the settlement.
- terrain raises access cost
- roads lower access cost
- roads extend reach but do not make it infinite
- seasonal transport modifiers do not change zoning legality

### Brush behavior
- brush accepts only legal cells for the chosen zone type
- illegal cells are ignored
- each contiguous legal painted area becomes one **zone component**
- one paint action can create multiple zone components if areas are disconnected

### Zone component as the simulation unit
Each zone component stores:
- owner settlement id
- zone type
- member cells
- total size
- access stats
- active/inactive data as required by zone type

### General zone rules
- minimum legal zone component size: 8 cells
- no hard maximum size
- zones can be deleted or repainted
- if roads or settlement expansion consume zoned cells, remove those cells immediately
- if that splits one component into multiple disconnected pieces, split immediately
- any resulting component under 8 cells is destroyed
- only the affected component recalculates

---

## 10. Farmland

### Farmland state model
Cell-level / zone-level farmland states:
- unzoned
- zoned farmland
- opening field
- active field
- abandoned field

### Farm plot model
Farmland zone components derive internal **farm plots**.
Farm plots are lightweight activation units, not worker containers.

Use farm plots only for:
- opening new land
- deciding which land remains active
- deciding which land becomes fallow first

Workers never belong to plots. Farming labor is assigned at settlement level each month.

### Plot generation rules
- generate as few plots as possible
- keep plots large and readable
- target size roughly 24-48 cells when possible
- minimum viable plot size: 12 cells
- plots must be contiguous
- roads, settlement footprint, and impassable cells break continuity
- avoid tiny leftovers where reasonably possible
- recalculation stays local to the affected farmland component

### Farm plot stored data
Each plot stores:
- parent farmland component id
- plot cells
- plot size
- average fertility
- average access cost
- forest clearing required flag
- state
- current-year labor coverage accumulator

### Farm plot states
- unopened
- opening
- active
- fallow

### Plot priority order
Sort plots by:
1. higher average fertility
2. lower average access cost
3. non-forested before forested
4. larger plot first

### Opening rules
- opening is plot-level
- non-forested plot: 1 month and 10 common work
- forested plot: 2 months and 15 common work
- reactivating a fallow plot: 1 month and 5 common work

### Seasonal labor profile per active plot
- January: 0
- February: 0
- March: 1
- April: 2
- May: 1
- June: 1
- July: 1
- August: 1
- September: 2
- October: 3
- November: 0
- December: 0

### Monthly farming labor assignment
Each month, assign farming labor to active plots in plot-priority order.
- best active plots receive labor first
- lower-priority active plots receive partial or zero labor when farming labor runs short

### Harvest
- harvest month: October
- base harvest per active plot:
  - 8 + (0.12 x average plot fertility)
- final harvest:
  - base harvest x annual field labor coverage x estate modifier
- annual field labor coverage:
  - total labor assigned from March-October / total labor required from March-October
  - clamp 0.0-1.0
- estate modifier:
  - 1.25 if settlement has staffed Estate I
  - otherwise 1.0

### Structural review timing
Only do structural farm changes in **November**.
At most **one structural farm-plot change per settlement per November**.

#### Retreat rule
If any active plot ended the harvest year with annual labor coverage below 1.0:
- the lowest-priority active plot becomes fallow

#### Expansion rule
If all active plots ended the harvest year with annual labor coverage 1.0 and the settlement has at least one fallow plot:
- the highest-priority fallow plot begins reactivation

If all active plots ended the harvest year with annual labor coverage 1.0, there are no fallow plots to reactivate, and the settlement has at least one unopened plot:
- the highest-priority unopened plot begins opening

### Immediate local recalculation on map changes
If a road or settlement expansion removes farmland cells:
- recalculate only the affected farmland component
- if a farm plot is cut into multiple pieces, any piece below 12 cells is destroyed immediately
- if at least one viable piece remains, the largest surviving piece keeps identity and other viable pieces become new plots
- unaffected plots keep identity

### Fertility
Fertility does not change over time in Alpha.

---

## 11. Forestry

### Valid forestry cell
A cell is valid for forestry only if:
- water tier 0
- slope 0-3
- forested

### Forestry model
- wood cells are binary in Alpha: forested or not forested
- forestry is renewable
- forestry is static renewable: no depletion and no regrowth timer
- output per fully worked active forest cell: **1.0 wood per month**

### Extra forested-cell project cost
Any project that must clear or prepare a forested cell gets:
- +1 month
- +5 common work

Forestry should use the normal zone-component model directly. No farm-plot-style substructure.

---

## 12. Quarry

### Valid quarry cell
A cell is valid for quarry only if:
- water tier 0
- slope 1-3
- fertility 25 or lower

### Quarry model
- quarries are infinite in Alpha
- output per fully worked active quarry cell: **0.6 stone per month**

Quarry uses the zone component directly as its simulation unit.

---

## 13. Resources and stockpiles

### Active shipping resources
Alpha shipping implementation actively simulates:
- food
- wood
- stone

### Deferred schema-only resources
Keep reserved ids and schema space for later use, but do not simulate them in Alpha:
- tools
- goods

### Stockpile model
Each settlement has its own local stockpile.
There is no global resource pool.
Zones are not stockpile nodes.
Only settlements store resources.

### Resource roles
- food: survival and population upkeep
- wood: roads, construction, expansion, founding
- stone: roads in rough terrain, construction, expansion, founding

---

## 14. Food and survival

### Food logic
- food is the survival resource
- no spoilage in Alpha
- food floor logic protects basic food staffing before most other extra roles collapse

### Shortage sequence
1. consume all food available after local supply and imports
2. if coverage is below full, cancel normal population growth
3. apply starvation by coverage band
4. remove lost population from the generic pool at month end

### Tuning areas
Implementation-owned:
- exact food consumption rate per population
- exact food-floor staffing target math
- Warehouse I storage value

These must remain simple and readable.

---

## 15. Logistics

### Logistics model
- graph-based
- settlements are nodes
- resources move only between connected settlements
- no teleportation
- no global pooling
- pull-based logistics
- imports happen within the same monthly logistics pass if path, surplus, and transport capacity exist

### Transport points
- each settlement has transport capacity each monthly pass
- transport points come from settlement scale, not a separate transport worker class
- roads reduce route cost strongly
- major water obstacles generally block land transport

### Pathing
Use cheapest valid land path between settlements.
Route cost depends on movement cost across cells using the same terrain and road cost model used elsewhere.

### Import/export flow
For each monthly logistics pass:
1. compute local production
2. compute local consumption and reservation needs
3. compute local deficits and surpluses
4. generate import requests for unresolved deficits
5. fulfill from connected settlements with exportable surplus along cheapest valid routes
6. consume transport points for the moved amount
7. leave unmet demand as shortage

### Local reserve rule
Before exporting, settlements should reserve enough for:
- food upkeep
- currently committed local construction materials if reserved
- other core local needs as defined by tuning

### Logistics priority order
When transport is scarce, fulfill in this order:
1. food upkeep
2. founding and core construction materials already reserved
3. other construction materials

### Tuning areas
Implementation-owned:
- exact transport point formula
- exact reserve thresholds
- exact tie-break logic among multiple candidate exporters
- exact multi-hop accounting details if multi-hop support is used

Implementation must preserve the locked logistics principles above.

---

## 16. Construction

### Construction model
Construction is queue-based and uses:
- materials
- common labor
- skilled labor

### Project families in shipping Alpha
- founding projects
- road projects
- automatic settlement expansion projects
- Estate I construction
- Warehouse I construction

### Hard rules
- player controls priorities, not individual workers
- reserve materials upfront, consume them gradually as progress advances
- one major internal building project active at once per settlement
- a settlement may queue multiple projects
- road projects are separate and progress cell by cell
- settlement expansion runs as background growth construction

### Project data model
Each project should track:
- project id
- owning settlement id
- project family and type
- route or target location if relevant
- priority
- status
- required materials
- reserved materials
- consumed materials
- remaining common work
- remaining skilled work
- access score or access modifier
- blockers
- expected completion estimate

### Project statuses
- queued
- waiting for materials
- waiting for labor
- in progress
- paused
- blocked
- completed
- canceled

### Construction flow
1. player issues order
2. system validates legality
3. project enters queue
4. materials reserve from local stockpile first
5. missing materials generate import demand if appropriate
6. labor is assigned automatically through normal labor allocation
7. progress occurs based on labor fill, materials ready state, and access
8. completion effects fire

### Cancellation
If canceled:
- unconsumed reserved materials return in full
- consumed materials are lost
- project progress is lost

### Construction and labor
- common labor mainly from serfs
- skilled labor mainly from artisans
- construction has no protected base floor
- construction should be the easiest labor to pull away from

### Tuning areas
Implementation-owned:
- building cost tables
- road cost tables by terrain
- project labor caps
- access modifiers on work speed
- exact work amounts for Estate I and Warehouse I

Keep tuning low-cardinality and readable.

---

## 17. Roads

### Road purpose
Roads do three things only:
1. reduce travel cost
2. improve access to nearby land and settlements
3. create a labor and material sink during construction

### Road rules
- one road type only
- no removal in Alpha
- no full bridge system
- major water crossings generally invalid
- roads occupy cells and make those cells unavailable for zoning while they exist

### Road project behavior
- player draws a route
- route is validated against terrain legality
- project proceeds from an accessible anchor outward
- road builds cell by cell
- each completed segment immediately uses road movement cost and can improve access for later segments

### Road side effects on zones
If a road consumes zoned cells:
- remove those cells from the affected zone immediately
- split components if disconnected
- destroy any new component below the legal minimum size
- if farmland plots are cut, apply the farmland recalculation rules above

---

## 18. Settlement expansion

### Expansion model
Settlement footprint expansion is automatic, formula-driven, and cannot be directly painted or aimed by the player.

### Development pressure
Monthly gain:
- max(0, population - 0.7 x current settlement cells)

Store pressure across months.

### Expansion project start requirements
A settlement may start an expansion-cell project only if:
- stored development pressure >= 30
- at least one valid adjacent candidate exists
- no other active settlement expansion cell project exists for that settlement
- local stockpile can reserve 2 wood and 1 stone

### Expansion candidate validity
A candidate cell is valid only if:
- it touches the current settlement footprint orthogonally or diagonally
- water tier 0
- slope 0-2
- not already in another settlement footprint

### Expansion scoring
For each valid candidate:
- cell score = 100 + (15 x orthogonal settlement neighbors) + (5 x diagonal settlement neighbors) - slope penalty - fertility penalty - farmland-preservation penalty

Slope penalty:
- slope 0 = 0
- slope 1 = 10
- slope 2 = 25

Fertility penalty:
- 0.5 x fertility

Farmland-preservation penalty:
- unzoned cell = 0
- abandoned field = 20
- zoned farmland = 25
- opening field = 40
- active field = 60

Pick the highest-scoring valid cell.
If all valid cells are farmland, still pick the highest-scoring valid cell.

### Expansion limits
- maximum completed expansion: 1 cell per settlement per month
- player cannot pause settlement expansion in Alpha

---

## 19. UI and player control surface

### Player actions
The player can:
- zone land
- draw road routes
- choose a settlement and queue Estate I or Warehouse I
- sponsor settlement founding from an existing settlement
- set priorities
- advance the month

### Required project feedback
For every project, UI must show:
- resource requirement
- labor requirement
- skill requirement
- current priority
- expected bottleneck

Every slow or blocked project must answer:
- why is this not building faster?

Possible reasons include:
- waiting for wood
- waiting for stone
- no spare serf labor
- no spare skilled labor
- poor access
- blocked by terrain
- paused by priority

### Visual priorities
- readability first
- low clutter
- minimal animation
- clear overlays for terrain, access, zones, roads, shortages, and project blockers

---

## 20. Engineering requirements

### Determinism
The sim must be deterministic for identical seed and player inputs.
Avoid frame-time-dependent logic.
Avoid unordered iteration over hash containers in outcome-critical systems unless iteration order is stabilized.

### Incremental recalculation
Prefer local recalculation only where possible:
- zoning changes recalc only affected components
- farmland replot only the affected farmland component
- road edits should update only impacted access caches if possible

### Save data
Save enough state to restore exactly:
- map seed and generated terrain or terrain data
- settlement state
- zone components
- farm plot state and annual labor coverage
- roads
- stockpiles
- labor and role state needed for continuity
- projects
- development pressure
- hidden fractional accumulators
- current month and year

### Performance guidance
Cache expensive path and access results where reasonable, but never at the cost of incorrect legality or stale logistics.
Design for the target monthly compute budget above.

---

## 21. Implementation-owned tuning

These are intentionally not locked and should be tuned in implementation and early playtests:
- zoning access / reach threshold math
- farmland plot partition heuristic
- food consumption values
- food floor thresholds
- Warehouse I capacity
- labor weights and reassignment limits
- construction cost tables
- project labor caps
- transport-capacity math
- exporter tie-break logic
- route-cost caching strategy

These tuning decisions must preserve the locked design principles in this spec.

---

## 22. Superseded items from older docs

These appeared in older materials but are not part of shipping Alpha:
- housing capacity
- overcrowding
- granary
- mill
- tool workshop
- goods workshop
- active tools production
- active goods production
- goods shortage gameplay
- extra settlement classes
- manual urban cell painting

Keep schema and code extensible enough that these can be added later without rewriting core terrain, zoning, logistics, and labor systems.
