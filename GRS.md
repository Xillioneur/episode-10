# Game Requirements Specification: Divine Sentinel

## 1. Functional Requirements
### 1.1 Emissary Controller (FR-EC)
*   **FR-EC-01:** Movement in 3D space using WASD keys.
*   **FR-EC-02:** Camera rotation via mouse movement with adjustable sensitivity.
*   **FR-EC-03:** Divine Actions: Left Click (Grace/Combo), Right Click (Heavy Blessing), Shift (Sprint/Celestial Step).
*   **FR-EC-04:** Divine Spirit (Stamina): Actions use Spirit; regeneration pauses during active blessings.
*   **FR-EC-05:** Restoration: Use 'E' to consume "Holy Essence" to restore Resolve.

### 1.2 Grace & Spirits (FR-GS)
*   **FR-GS-01:** AI State Machine: Wander -> Confused -> Agitated -> Seeking Clarity.
*   **FR-GS-02:** Restoration Logic: Tracking "Clarity" (Poise) damage and triggering "Ascension" states.
*   **FR-GS-03:** Moment of Peace (Hit-stop): Global timescale reduction (0.05x) for 0.08s upon successful blessings.
*   **FR-GS-04:** Ascension: When a spirit is purified, it turns into golden butterflies/particles and rises upward.

### 1.3 Flow of Grace (FR-FG)
*   **FR-FG-01:** Ascension Portal: Move to the next area once all spirits have been purified.
*   **FR-FG-02:** Renewal Condition: Transition to the "Returning to Light" screen if Resolve is depleted.

## 2. Non-Functional Requirements
### 2.1 Aesthetic & Feel (NFR-AF)
*   **NFR-AF-01:** Color Palette: Must prioritize Gold, White, and Sky Blue.
*   **NFR-AF-02:** Feedback: All effects must feel "soft" and "resplendent" rather than "sharp" or "violent."

### 2.2 Performance (NFR-P)
*   **NFR-P-01:** Maintain 60 FPS.
*   **NFR-P-02:** Smooth camera transitions during target locking.
