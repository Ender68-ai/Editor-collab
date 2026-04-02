# CollabEditor Mod - Development Roadmap

## Milestone 1: The Great Bug Hunt ✅ COMPLETE
Foundation stability before new features.

- [x] Resolve Compilation Errors
  - [x] Update all matjson calls in CollabManager.cpp to camelCase API
  - [x] Add default constructor to CollabLevel
  - [x] Properly declare m_data in popup headers
  - [x] Fix `CCDirector` error: Replace `getFrameSinceWindowsOpened()` with `getTotalFrames()`
  - [x] Fix Header Error: Rename `LocalLevelLayer` hook to `LevelInfoLayer`

- [x] Fix the Level Hook
  - [x] Remove invalid LevelCell::onRightClick(sender) call in LevelMenu.cpp
  - [x] Replace with custom button added via CCMenu inside LevelCell hook

- [x] Clean Compile
  - [x] Verify mod builds on Clang without errors (verified on i3-10th Gen)

- [x] UI Integration - LevelInfoLayer Button
  - [x] Add CollabSettingsPopup with E/D real-time collab toggle
  - [x] Create button with GJ_shareBtn_001.png sprite at {260.f, 55.f}

---

## Milestone 2: Data Persistence (CollabManager)
Ensure local saves and loads work correctly.

- [ ] Save Logic Testing
  - [ ] Test hostedLevels serialization to Geode save file
  - [ ] Test collabedLevels serialization
  - [ ] Test collabLevels serialization
  - [ ] Test transferRequests serialization

- [ ] Load Logic Testing
  - [ ] Verify std::unordered_map reconstruction on game launch
  - [ ] Verify arrays load correctly with all data intact

- [ ] First Time Setup
  - [ ] Finalize m_firstTimeSetup logic
  - [ ] Show welcome popup or tutorial on first load

---

## Milestone 3: The User Interface (CollabPopups & Hooks)
Build seamless in-game interaction.

- [ ] **LevelInfoLayer Button Overhaul (Priority)**
  - [ ] **Visuals:** Create `CCMenuItemSpriteExtra` using `GJ_button_01.png` (green circle) as base.
  - [ ] **Layering:** Add `collab_logo.png` as a child centered on the button base.
  - [ ] **Permissions:** Logic to only show if `m_level->m_levelType == GJLevelType::Editor`.
  - [ ] **Placement:** Align exactly 35 units above `m_statsBtn` in the bottom right menu.
  - [ ] **Stability:** Remove manual `release()` calls to fix crash on layer exit.

- [ ] Host Popup
  - [ ] Create UI to turn local level into hosted collab
  - [ ] Add input field for level name
  - [ ] Add list to add collaborator Account IDs

- [ ] Collab Details Popup
  - [ ] Show current collab status
  - [ ] Display host name
  - [ ] Display list of collaborators

- [ ] Menu Hooks
  - [ ] Inject collab buttons into LevelBrowserLayer
  - [ ] Ensure no vanilla button overlaps

---

## Milestone 4: Networking & Transfers (P2P Phase)
Core multiplayer functionality.

- [ ] **Local P2P Testing**
  - [ ] Test connection between PC and Mobile (same network)
  - [ ] Implement `getTotalFrames()` / 60.0f timestamping for sync

- [ ] Level Handoff Logic
  - [ ] Package local level string safely
  - [ ] Upload level data
  - [ ] Send transfer request to target Account ID

---

## Milestone 5: Pre-Flight Checklist (v1.0.0 Release)
Final polish and release preparation.

- [ ] **Performance Profile**
  - [ ] Test build performance on Ryzen 3 3100 / Ryzen 7 5700X
  - [ ] Optimize compile times (remove unnecessary `rd /s /q` from .bat)

- [ ] Assets & Metadata
  - [ ] Design final white-alpha `collab_logo.png`
  - [ ] Design white-alpha `cursor.png` and `crosshair.png`

---

## Completed Tasks
- [x] Remove duplicate message display in ChatLayer
- [x] Fix CollabPopups TransferRequestPopup default constructor
- [x] Fix CollabManager matjson array handling (use .push())
- [x] Fix message positioning in ChatLayer scroll area
- [x] Successfully compiled P2P timing logic on i3-10th Gen