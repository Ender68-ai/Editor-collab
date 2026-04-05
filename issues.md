# Known Issues - CollabEditor

## Issue #1: Collab Settings Button Uses Default Sprite
**Priority**: Medium
**Description**: The collaboration settings button currently uses `GJ_shareBtn_001.png` as a fallback/default sprite. A custom sprite should be created for better visual identification and branding.

**Location**: `src/LocalLevelLayer.cpp` line 18
**Current Code**:
```cpp
CCSprite* btnSprite = CCSprite::createWithSpriteFrameName("GJ_shareBtn_001.png");
```

**Proposed Solution**: 
- Create a custom "collab" icon sprite (e.g., `collabIcon.png` or `GJ_collabBtn_001.png`)
- Add it to the mod resources in `mod.json`
- Update the sprite loading code to use the custom sprite

---

## Issue #2: Button Only Appears in Uploaded Levels
**Priority**: High
**Description**: The collab settings button currently only appears in levels that have already been uploaded to the Geometry Dash servers. It does not appear in:
- Local/unpublished levels
- Newly created levels
- Levels saved locally but not uploaded

**Location**: `src/LocalLevelLayer.cpp` - `CollabLevelInfoLayerExt::init()`

**Expected Behavior**: The button should appear in ALL levels, regardless of upload status, since collaboration can happen during the creation phase before uploading.

**Proposed Solution**:
- Review the hook/modify target - may need to hook a different class or method
- Check if the issue is related to `LevelInfoLayer` vs `LevelBrowserLayer`
- Consider adding the button to the editor UI as well (`EditorUI` hook)

---

## Issue #3: Settings Are Minimal
**Priority**: Medium
**Description**: The current collaboration settings are very basic and lack important functionality:

**Missing Features**:
- No role management UI (Editor/Suggestor/Viewer roles exist in code but not in UI)
- No player invitation system
- No way to view or manage current collaborators
- No level transfer confirmation dialog UI
- No settings for sync frequency or conflict resolution
- No "kick player" functionality
- No way to view transfer request history

**Location**: 
- `src/CollabPopups.cpp` - `CollabSettingsPopup`
- `src/CollabManager.hpp` - Data structures exist but UI is lacking

**Proposed Enhancements**:
1. **CollabSettingsPopup Overhaul**:
   - Add player list with current collaborators
   - Add "Invite Player" button with username/account ID input
   - Add role dropdown for each player
   - Add kick/remove player button
   
2. **Transfer System UI**:
   - Add proper transfer request popup
   - Add transfer history/log
   
3. **Advanced Settings**:
   - Sync frequency options
   - Auto-save intervals
   - Notification preferences

---

## Additional Notes

### Testing Checklist
- [ ] Verify button appears in local levels
- [ ] Verify button appears in uploaded levels
- [ ] Verify button appears in editor mode
- [ ] Test custom sprite rendering
- [ ] Test all settings popup functionality

### Related Files
- `src/LocalLevelLayer.cpp` - Button placement
- `src/CollabPopups.cpp` - Popup UIs
- `src/CollabManager.hpp/cpp` - Backend logic
- `mod.json` - Resource management
