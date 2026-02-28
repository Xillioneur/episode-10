# Game Requirements Specification: Divine Sentinel

## 1. Functional Requirements
### 1.1 Sanctuary Statues (FR-SS)
- **FR-SS-01:** Interaction: Press 'F' when within 10.0f units of an `OBS_STATUE` to open the Sanctuary Menu.
- **FR-SS-02:** Resting: Fully restore `player.health` and `player.flasks` upon opening the menu.
- **FR-SS-03:** Attunement: Use Relics as currency to purchase upgrades (Cost: 3 per level).

### 1.2 Radiant Empowerment (FR-RE)
- **FR-RE-01:** Detect if the player's horizontal position is within 15.0f units of any active light shaft center.
- **FR-RE-02:** Apply a 1.5x multiplier to `STAMINA_REGEN_RATE` while empowered.

### 1.3 Upgrades (FR-UP)
- **FR-UP-01:** Mercy Upgrade: +1 Holy Essence capacity per level.
- **FR-UP-02:** Discipline Upgrade: +10% Poise damage per level.
- **FR-UP-03:** Fortitude Upgrade: +25 Max Resolve per level.

## 2. Technical Constraints
- The game state must transition to `SANCTUARY` during interaction, pausing entity updates but allowing particle/camera movement.
