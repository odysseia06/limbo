# Limbo Editor User Guide

The Limbo Editor is a visual tool for creating and editing game scenes.

## Getting Started

```bash
./build/bin/limbo_editor
```

## Interface Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  File  Edit  Entity  View  Help                                     │
├─────────────────────────────────────────────────────────────────────┤
│  [Play] [Pause] [Stop]                    Scene: Untitled           │
├──────────────┬────────────────────────────────┬─────────────────────┤
│  Hierarchy   │         Viewport               │     Inspector       │
│              │                                │                     │
│  > Parent    │    ┌───────────────────┐       │  Entity1            │
│    > Child1  │    │   [Gizmo]         │       │  ──────────         │
│    > Child2  │    │     ↑             │       │  Transform          │
│  > Entity3   │    │   ←─┼─→           │       │  Position: 0, 0, 0  │
│              │    │     ↓             │       │  Rotation: 0, 0, 0  │
│              │    └───────────────────┘       │  Scale: 1, 1, 1     │
│              │                                │                     │
│              │                                │  [Add Component]    │
├──────────────┴────────────────────────────────┴─────────────────────┤
│                        Asset Browser                                 │
│  <- [Home] [Refresh]  [Search...]                                   │
│  [D] textures   [D] scripts   [D] scenes   [I] player.png           │
├─────────────────────────────────────────────────────────────────────┤
│  FPS: 60 | Entities: 3 | Draw Calls: 1 | Quads: 3                   │
└─────────────────────────────────────────────────────────────────────┘
```

The layout is automatically saved to `limbo_editor.ini` and restored on launch.

## Panels

### Hierarchy Panel

Shows all entities as a tree with parent-child relationships.

- Click to select
- Drag entities to reparent
- Right-click for context menu (Create, Delete, Duplicate)

### Inspector Panel

Edit properties of the selected entity.

- **Name**: Editable entity name
- **Transform**: Position, Rotation, Scale
- **Components**: Collapsible sections with X to remove
- **Add Component**: Button to add new components

### Viewport Panel

Visual scene view with gizmos and camera controls.

**Gizmo Modes** (keyboard shortcuts):
| Key | Mode |
|-----|------|
| W | Translate |
| E | Rotate |
| R | Scale |
| Ctrl | Hold for snapping |

**Camera Controls**:
| Input | Action |
|-------|--------|
| WASD/Arrows | Pan camera |
| Mouse Scroll | Zoom |
| Home | Reset camera |

### Asset Browser

Browse and manage project assets.

- **Search**: Filter assets by name
- **Drag-drop**: Drag images to viewport to create sprites
- **Navigation**: Back, Home, Refresh buttons
- **Color-coded icons**: Directories (yellow), Images (green), JSON (blue), Lua (dark blue)

## Undo/Redo

Full undo/redo support for common operations:

| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

Supported operations:
- Entity create/delete/duplicate/rename
- Entity reparenting
- Transform changes
- Component add/remove
- Property edits

The Edit menu shows the next undo/redo action description.

## Play Mode

Test your scene without permanently modifying it.

| Button | Action |
|--------|--------|
| Play | Enter play mode (systems run) |
| Pause | Pause/resume simulation |
| Stop | Exit and restore scene state |

Scene state is automatically saved before play and restored when stopped.

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New scene |
| Ctrl+O | Open scene |
| Ctrl+S | Save scene |
| Ctrl+Shift+S | Save as |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Delete | Delete entity |
| W | Translate gizmo |
| E | Rotate gizmo |
| R | Scale gizmo |
| Ctrl (hold) | Snapping |
| Home | Reset camera |
| F1 | Toggle demo window |

## Workflow

### Creating a Scene

1. **Entity > Create Sprite** to add entities
2. Select and edit in Inspector
3. Use gizmos (W/E/R) to position
4. **File > Save** to save

### Using Prefabs

1. Create and configure an entity
2. Right-click > **Create Prefab**
3. Drag prefab from Asset Browser to create instances
4. Edit instances - overrides are preserved

### Testing with Play Mode

1. Click **Play** to start simulation
2. Observe physics and systems running
3. Click **Stop** to restore original state
4. Changes during play are discarded unless explicitly saved

## See Also

- [Editor Workflows](EDITOR_WORKFLOWS.md) - Detailed workflow guide
- [Scenes and Prefabs](SCENES_AND_PREFABS.md) - Prefab system documentation
