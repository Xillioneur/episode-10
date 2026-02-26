# Game Requirements Specification: Divine Sentinel

## 1. Functional Requirements
### 1.1 Relic Collection (FR-RC)
- **FR-RC-01:** Automatically spawn a `RelicOrb` at the spirit's location upon purification (0 health).
- **FR-RC-02:** Detect player collision with `RelicOrb` to add it to the inventory.
- **FR-RC-03:** Relic type is determined by the spirit's `SpiritTrait`.

### 1.2 Sacred Inventory (FR-SI)
- **FR-SI-01:** Toggle inventory screen with the 'Tab' key.
- **FR-SI-02:** Display counts for Relics of Mercy, Discipline, and Fortitude.
- **FR-SI-03:** Apply stacking passive buffs based on inventory counts (e.g., +5% Health per Fortitude Relic).

### 1.3 World Interaction (FR-WI)
- **FR-WI-01:** Render floating, rotating spheres for Relic Orbs in the 3D scene.

## 2. Technical Constraints
- The inventory system must persist across level transitions within a single session.
