# Limbo Editor User Guide

The Limbo Editor is a visual tool for creating and editing game scenes.

## Getting Started

### Launching the Editor

```bash
./build/bin/limbo_editor
```

The editor opens with an empty scene and the default panel layout.

### Interface Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  File  Edit  Entity  View  Help                                     │
├─────────────────────────────────────────────────────────────────────┤
│  [Play] [Pause] [Stop]                    Scene: Untitled           │
├──────────────┬────────────────────────────────┬─────────────────────┤
│  Hierarchy   │         Viewport               │     Inspector       │
│              │                                │                     │
│  > Entity1   │    ┌───────────────────┐       │  Entity1            │
│  > Entity2   │    │                   │       │  ──────────         │
│  > Entity3   │    │     [Grid]        │       │  Name: Entity1      │
│              │    │                   │       │                     │
│              │    │                   │       │  Transform          │
│              │    └───────────────────┘       │  Position: 0, 0, 0  │
│              │                                │  Rotation: 0, 0, 0  │
│              │                                │  Scale: 1, 1, 1     │
│              │                                │                     │
│              │                                │  [Add Component]    │
├──────────────┴────────────────────────────────┴─────────────────────┤
│                        Asset Browser                                 │
│  <- C:/project/assets                                               │
│  [D] textures   [D] scripts   [D] scenes   [I] player.png           │
├─────────────────────────────────────────────────────────────────────┤
│  FPS: 60 | Entities: 3 | Draw Calls: 1 | Quads: 3                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Panels

### Hierarchy Panel

Shows all entities in the current scene as a tree view.

**Features:**
- Click to select an entity
- Right-click for context menu (Delete, Duplicate)
- Right-click empty space to create new entities

### Inspector Panel

Displays and edits properties of the selected entity.

**Sections:**
- **Name**: Entity name (editable text field)
- **Components**: Collapsible sections for each component
  - X button to remove component
  - Drag fields to edit values
- **Add Component**: Button to add new components

**Editable Components:**
- Transform (Position, Rotation, Scale)
- Sprite Renderer (Color)
- Rigidbody 2D (Body Type, Fixed Rotation)
- Box Collider 2D (Size, Offset, Physics properties)
- Circle Collider 2D (Radius, Offset, Physics properties)

### Viewport Panel

Visual representation of the scene.

**Camera Controls:**
| Input | Action |
|-------|--------|
| W/Up | Pan camera up |
| S/Down | Pan camera down |
| A/Left | Pan camera left |
| D/Right | Pan camera right |
| Mouse Scroll | Zoom in/out |
| Home | Reset camera |

**Display:**
- Grid overlay with axis indicators (red = X, green = Y)
- All sprite entities rendered
- Camera position and zoom preserved while editing

### Asset Browser Panel

Browse project assets for importing into scenes.

**Features:**
- Navigate folders with back button and double-click
- Thumbnail grid with icons:
  - [D] Directory (yellow)
  - [I] Image file (green)
  - [J] JSON file (blue)
  - [L] Lua script (dark blue)
  - [S] Shader file (orange)
  - [A] Audio file (pink)
- Adjustable thumbnail size slider
- Right-click for context menu (Delete, Rename, Show in Explorer)
- Drag and drop support (future)

## Menu Bar

### File Menu

| Command | Shortcut | Description |
|---------|----------|-------------|
| New Scene | Ctrl+N | Clear scene and start fresh |
| Open Scene | Ctrl+O | Load a scene file |
| Save | Ctrl+S | Save current scene |
| Save As | Ctrl+Shift+S | Save to new file |
| Exit | Alt+F4 | Close editor |

### Edit Menu

| Command | Shortcut | Description |
|---------|----------|-------------|
| Undo | Ctrl+Z | (Not yet implemented) |
| Redo | Ctrl+Y | (Not yet implemented) |
| Delete | Delete | Delete selected entity |

### Entity Menu

| Command | Description |
|---------|-------------|
| Create Empty | New entity with Transform only |
| Create Sprite | New entity with Transform and Sprite |
| Create Camera | New entity with Transform and Camera |

### View Menu

Toggle visibility of panels:
- Hierarchy
- Inspector
- Viewport
- Asset Browser
- ImGui Demo (F1)

## Play Mode

The toolbar provides Play/Pause/Stop controls:

| Button | State | Description |
|--------|-------|-------------|
| Play | Edit mode | Start simulating the scene |
| Playing | Play mode | Scene is running (green highlight) |
| Pause | Play mode | Pause simulation |
| Paused | Paused | Simulation paused (yellow highlight) |
| Stop | Play/Pause | Return to edit mode |

**Play Mode Behavior:**
- Physics simulation runs
- Systems execute (animation, particles, etc.)
- Entities can be observed but editing is limited
- Stopping restores the scene to its pre-play state (planned)

## Workflow

### Creating a New Scene

1. **File > New Scene** or Ctrl+N
2. Create entities via **Entity** menu
3. Select entities in Hierarchy
4. Edit properties in Inspector
5. Position using Transform values
6. Save with **File > Save As**

### Building a Simple Level

1. Create a ground:
   - Entity > Create Sprite
   - Set Transform Scale to (10, 0.5, 1)
   - Set Transform Position Y to -2
   - Set Sprite color to gray

2. Create a player:
   - Entity > Create Sprite
   - Set Transform Scale to (0.5, 0.5, 1)
   - Set Sprite color to green

3. Add physics:
   - Select ground, Add Component > Rigidbody 2D
   - Set Body Type to Static
   - Add Component > Box Collider 2D

4. Add player physics:
   - Select player, Add Component > Rigidbody 2D
   - Body Type stays Dynamic
   - Add Component > Box Collider 2D

5. Test with Play button

### Saving and Loading

**Save Scene:**
```
File > Save As
Enter filename: level1.json
Saves to: assets/scenes/level1.json
```

**Load Scene:**
```
File > Open Scene
Loads: assets/scenes/default.json
```

**In Code:**
```cpp
limbo::SceneSerializer serializer(getWorld());
serializer.loadFromFile("assets/scenes/level1.json");
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New scene |
| Ctrl+O | Open scene |
| Ctrl+S | Save scene |
| Ctrl+Shift+S | Save scene as |
| Delete | Delete selected entity |
| F1 | Toggle ImGui demo window |
| Escape | Exit (from any app) |
| Home | Reset viewport camera |

## Status Bar

Displays real-time information:
- **FPS**: Frames per second
- **Entities**: Number of entities in scene
- **Draw Calls**: Renderer batch count
- **Quads**: Number of sprites rendered

## Tips and Tricks

1. **Quick Iteration**: Use Play mode to test physics without leaving the editor

2. **Precise Positioning**: Type exact values in Transform fields instead of dragging

3. **Component Order**: Add Rigidbody2D before colliders for proper physics setup

4. **Zoom for Detail**: Scroll to zoom in for precise placement

5. **Grid Alignment**: Use the grid as reference (1 unit spacing by default)

6. **Asset Organization**: Keep assets in subfolders (textures/, scripts/, scenes/)

## Known Limitations

- No undo/redo yet
- No multi-select
- No copy/paste entities
- No scene hierarchy (parent-child)
- No gizmo manipulation (drag handles)
- No asset import dialogs (files must be placed manually)
- Play mode doesn't save/restore state yet

## Future Features

- Transform gizmos (move, rotate, scale handles)
- Asset import dialogs
- Undo/redo system
- Multi-select and group operations
- Prefab system
- Scene hierarchy (entity parenting)
- Custom component editors
- Console/log panel
- Tilemap editor integration
