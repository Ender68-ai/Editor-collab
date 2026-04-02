# CollabEditor Mod - Development Roadmap

## Milestone 1: The Great Bug Hunt (Current State) ✅ COMPLETE
Foundation stability before new features.

- [x] Resolve Compilation Errors
  - [x] Update all matjson calls in CollabManager.cpp to new camelCase API (asArray(), asBool(), isNull())
  - [x] Add default constructor to CollabLevel to fix std::unordered_map template errors
  - [x] Properly declare m_data in popup headers (CollabPopups.hpp)

- [x] Fix the Level Hook
  - [x] Remove invalid LevelCell::onRightClick(sender) call in LevelMenu.cpp
  - [x] Replace with custom button added via CCMenu inside LevelCell hook

- [x] Clean Compile
  - [x] Verify mod builds on Clang without errors (warnings only from external libs)

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

- [ ] Host Popup
  - [ ] Create UI to turn local level into hosted collab
  - [ ] Add input field for level name
  - [ ] Add list to add collaborator Account IDs

- [ ] Collab Details Popup
  - [ ] Show current collab status
  - [ ] Display host name
  - [ ] Display list of collaborators
  - [ ] Show current level version/ID

- [ ] Transfer Request Popup
  - [ ] Build inbox/alert UI
  - [ ] Implement accept button functionality
  - [ ] Implement deny button functionality

- [ ] Menu Hooks
  - [ ] Inject collab buttons into LevelBrowserLayer
  - [ ] Inject collab buttons into LevelInfoLayer
  - [ ] Ensure no vanilla button overlaps

---

## Milestone 4: Networking & Transfers
Core multiplayer functionality.

- [ ] API / Web Requests
  - [ ] Implement Geode web::AsyncWebRequest logic
  - [ ] Add logic to fetch collab data
  - [ ] Verify Account IDs
  - [ ] Push level data to server (if using custom backend)

- [ ] Level Handoff Logic
  - [ ] Package local level string safely
  - [ ] Upload level data
  - [ ] Send transfer request to target Account ID

- [ ] Error Handling
  - [ ] Add error popup for offline players
  - [ ] Add error popup for invalid Account IDs
  - [ ] Add error popup for server timeouts
  - [ ] Display errors using FLAlertLayer

---

## Milestone 5: Pre-Flight Checklist (v1.0.0 Release)
Final polish and release preparation.

- [ ] Playtesting
  - [ ] Delete save file and test from fresh state
  - [ ] Input garbage text into ID fields
  - [ ] Simulate network drops
  - [ ] Test all error scenarios

- [ ] Code Cleanup
  - [ ] Remove leftover CCLOG statements
  - [ ] Remove leftover log::info spam
  - [ ] Review code for debugging artifacts

- [ ] Assets & Metadata
  - [ ] Design cool logo.png for mod
  - [ ] Fill out about.md with usage instructions
  - [ ] Update mod.json with v1.0.0 version
  - [ ] Update mod.json description
  - [ ] Update mod.json developer tags

- [ ] Final Release Build
  - [ ] Compile mod in Release mode
  - [ ] Test .geode file on clean Geometry Dash installation
  - [ ] Verify all features work end-to-end

---

## Completed Tasks
- [x] Remove duplicate message display in ChatLayer
- [x] Fix CollabPopups TransferRequestPopup default constructor
- [x] Fix CollabManager matjson array handling (use .push() instead of .push_back())
- [x] Fix message positioning in ChatLayer scroll area
