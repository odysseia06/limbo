# Limbo Editor Workflows

This document describes the common workflows and features available in the Limbo Editor.

## Table of Contents

- [Getting Started](#getting-started)
- [Editor Layout](#editor-layout)
- [Entity Management](#entity-management)
- [Transform Gizmos](#transform-gizmos)
- [Undo/Redo System](#undo-redo-system)
- [Play Mode](#play-mode)
- [Asset Browser](#asset-browser)
- [Keyboard Shortcuts](#keyboard-shortcuts)

## Getting Started

Launch the editor executable (`limbo_editor`). The editor will open with a default docking layout that includes:

- **Hierarchy Panel** - Shows all entities in the scene
- **Inspector Panel** - Shows properties of the selected entity
- **Viewport Panel** - 2D scene view with camera controls
- **Asset Browser** - Browse and manage project assets
- **Asset Pipeline** - Monitor asset import/build status

The layout is automatically saved to `limbo_editor.ini` and restored on next launch.

## Editor Layout

### Docking System

All panels support ImGui docking:

- **Drag panels** by their title bar to dock them in different locations
- **Tabs** are created when panels are docked in the same location
- **Resize** panels by dragging the borders between them
- Layout changes are automatically persisted

### Panel Visibility

Toggle panel visibility through the **View** menu or by closing panels with the X button.

## Entity Management

### Creating Entities

From the **Entity** menu:

- **Create Empty** - Creates an entity with only a TransformComponent
- **Create Sprite** - Creates an entity with Transform and SpriteRendererComponent
- **Create Camera** - Creates an entity with Transform and CameraComponent

Or right-click in the Hierarchy panel for a context menu.

### Selecting Entities

- **Click** on an entity in the Hierarchy panel to select it
- Selected entities show an orange outline in the viewport
- Selection is shown in the Inspector panel

### Deleting Entities

- Select an entity and press **Delete** key
- Or use **Edit > Delete** menu
- Or right-click the entity and select **Delete**

### Hierarchy (Parent-Child)

Entities can be organized in hierarchies:

- **Drag and drop** entities in the Hierarchy panel to reparent them
- Child transforms are relative to their parent
- Deleting a parent also deletes all children

### Duplicating Entities

- Right-click an entity and select **Duplicate**
- Creates a copy with "_Copy" suffix

## Transform Gizmos

The viewport displays interactive gizmos for manipulating selected entities.

### Gizmo Modes

Switch between modes using keyboard shortcuts or the toolbar:

| Mode | Key | Description |
|------|-----|-------------|
| Translate | **W** | Move the entity along X/Y axes |
| Rotate | **E** | Rotate the entity around Z axis |
| Scale | **R** | Scale the entity uniformly or per-axis |

### Using Gizmos

1. Select an entity
2. Choose a gizmo mode (W/E/R)
3. Click and drag on the gizmo handles:
   - **Red arrow/handle** - X axis
   - **Green arrow/handle** - Y axis
   - **Center square** - Both axes (XY)

### Snapping

Hold **Ctrl** while dragging to enable snapping:

- **Translate**: Snaps to 0.5 unit increments
- **Rotate**: Snaps to 15 degree increments
- **Scale**: Snaps to 0.1 unit increments

## Undo/Redo System

The editor tracks all changes and supports unlimited undo/redo.

### Supported Operations

The following actions can be undone:

- Entity creation/deletion
- Entity reparenting
- Entity duplication
- Entity renaming
- Transform changes (position, rotation, scale)
- Component property changes
- Component add/remove

### Using Undo/Redo

| Action | Shortcut | Menu |
|--------|----------|------|
| Undo | **Ctrl+Z** | Edit > Undo |
| Redo | **Ctrl+Y** or **Ctrl+Shift+Z** | Edit > Redo |

The Edit menu shows the description of the next undo/redo action.

### Command Merging

Rapid sequential changes of the same type are merged into a single undo operation. For example, dragging a slider creates one undo entry for the entire drag, not one per frame.

## Play Mode

Play mode allows you to test your scene at runtime without permanently modifying it.

### Controls

| Button | Action |
|--------|--------|
| **Play** | Enter play mode - scene simulation begins |
| **Pause** | Pause/resume simulation |
| **Stop** | Exit play mode - scene is restored to pre-play state |

### Behavior

When entering Play mode:

1. The current scene state is saved
2. Systems begin updating (physics, scripts, etc.)
3. The undo history is cleared

When stopping Play mode:

1. The scene is restored to its pre-play state
2. Any changes made during play are discarded

### Persisting Play Mode Changes

If you make changes during play mode that you want to keep:

1. Note the values you want to preserve
2. Stop play mode
3. Manually apply the changes in edit mode
4. Save the scene

## Asset Browser

The Asset Browser provides a visual interface for managing project assets.

### Navigation

- **Back button (<-)** - Go to parent directory
- **Home button** - Return to assets root
- **Refresh button** - Rescan current directory
- **Double-click folder** - Enter directory

### Search

Use the search box to filter assets by name:

- Search is case-insensitive
- Matches partial names
- Click **X** to clear the search

### Asset Types

Assets are color-coded by type:

| Color | Type |
|-------|------|
| Yellow | Directories |
| Green | Images (.png, .jpg, .bmp, .tga) |
| Blue | JSON files (.json) |
| Dark Blue | Lua scripts (.lua) |
| Orange | Shaders (.glsl, .vert, .frag) |
| Pink | Audio (.wav, .mp3, .ogg) |
| Light Purple | Fonts (.ttf, .otf) |
| Cyan | Prefabs (.prefab) |

### Drag and Drop

Drag assets from the browser to the viewport:

- **Images** - Creates a sprite entity at the drop location
- **Prefabs** - Instantiates the prefab at the drop location

### Context Menu

Right-click assets for options:

- **Open** - Open directory or asset
- **Delete** - Remove the asset (with confirmation)
- **Rename** - Rename the asset
- **Show in Explorer** - Open in system file browser

## Keyboard Shortcuts

### File Operations

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New Scene |
| Ctrl+O | Open Scene |
| Ctrl+S | Save Scene |
| Ctrl+Shift+S | Save Scene As |

### Edit Operations

| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+Shift+Z | Redo (alternative) |
| Delete | Delete selected entity |

### Viewport

| Shortcut | Action |
|----------|--------|
| W | Translate gizmo mode |
| E | Rotate gizmo mode |
| R | Scale gizmo mode |
| Ctrl (hold) | Enable snapping |
| Home | Reset camera position and zoom |
| Mouse Wheel | Zoom in/out |
| WASD / Arrow Keys | Pan camera (when viewport focused) |

### Panels

| Shortcut | Action |
|----------|--------|
| F1 | Toggle ImGui Demo Window |

## Tips and Best Practices

### Organizing Scenes

- Use parent entities as folders to group related objects
- Name entities descriptively
- Use prefabs for reusable entity configurations

### Performance

- The status bar shows FPS, entity count, and draw call statistics
- Use the Asset Pipeline panel to monitor asset processing

### Workflow Efficiency

- Learn the keyboard shortcuts for common operations
- Use the gizmo snapping (Ctrl) for precise placement
- Test frequently with Play mode before final save

### Saving Your Work

- The editor shows an asterisk (*) next to modified scene names
- Save frequently with Ctrl+S
- The layout is automatically saved, but scene changes require explicit save
