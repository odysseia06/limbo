# Performance & Profiling Guide

This document covers the performance infrastructure in Limbo Engine, including profiling tools, memory management, and threading APIs.

## CPU Profiler

The engine includes a hierarchical CPU profiler for measuring frame timing.

### Basic Usage

```cpp
#include "limbo/debug/Profiler.hpp"

// Profile a scope
void MySystem::update(World& world, f32 dt) {
    LIMBO_PROFILE_SCOPE("MySystem::update");
    
    {
        LIMBO_PROFILE_SCOPE("Physics");
        // Physics work...
    }
    
    {
        LIMBO_PROFILE_SCOPE("AI");
        // AI work...
    }
}

// Profile a function (uses function name automatically)
void processEntities() {
    LIMBO_PROFILE_FUNCTION();
    // ...
}
```

### Capturing and Exporting Data

```cpp
// Capture current frame for analysis
profiler::Profiler::captureFrame();

// Get captured frame data
const profiler::FrameData* frame = profiler::Profiler::getCapturedFrame();
for (const auto& sample : frame->samples) {
    printf("%s: %.3f ms\n", sample.name, sample.getDurationMs());
}

// Export to CSV
profiler::Profiler::exportToCSV("profiler_data.csv");
```

### Profiler Panel

In the editor, use `DebugPanels::showProfilerPanel()` to display:
- Frame time and sample count
- CPU timeline visualization
- Sample details table with duration and percentage
- Frame allocator and thread pool stats

## Frame Allocator

A bump-pointer allocator reset each frame, eliminating per-frame heap allocations.

### Usage

```cpp
#include "limbo/core/FrameAllocator.hpp"

// Allocate an array (no constructors called - for POD types)
auto* vertices = frame::allocateArray<Vertex>(1000);

// Allocate and construct an object
auto* config = frame::create<RenderConfig>(width, height);

// Use FrameVector instead of std::vector for temporary data
FrameVector<Entity> entities;
entities.reserve(100);
for (auto e : view) {
    entities.push_back(e);
}
```

### Important Notes

- Memory is only valid for the current frame
- Do NOT store pointers to frame-allocated data across frames
- The allocator is automatically reset at the start of each frame
- Default capacity is 1MB; adjust in `Application::init()` if needed

### Monitoring Usage

```cpp
auto& allocator = frame::get();
printf("Used: %zu KB / %zu KB\n", 
       allocator.getUsedBytes() / 1024,
       allocator.getCapacity() / 1024);
printf("Peak: %zu KB\n", allocator.getPeakUsage() / 1024);
```

## Thread Pool

A worker thread pool for parallel execution of background tasks.

### Thread Safety Rules

**NEVER call these from worker threads:**
- OpenGL/GLFW/ImGui functions
- Entity creation/destruction
- Component modification

**SAFE to call from worker threads:**
- File I/O
- Image/audio decoding
- Math computations
- Parsing (JSON, etc.)

### Basic Usage

```cpp
#include "limbo/core/ThreadPool.hpp"

// Submit a job
auto future = ThreadPool::submit([]() {
    // Background work...
    return computeResult();
});

// Wait for result
auto result = future.get();

// Wait for all jobs
ThreadPool::waitAll();

// Check if on main thread
if (ThreadPool::isMainThread()) {
    // Safe to call OpenGL...
}
```

### Main Thread Queue

For work that must run on the main thread (GPU uploads, etc.):

```cpp
#include "limbo/core/MainThreadQueue.hpp"

// From a worker thread:
MainThreadQueue::enqueue([texture, data]() {
    texture->upload(data);  // OpenGL call - safe on main thread
});

// In main loop (called automatically by Application):
MainThreadQueue::processAll();
```

## Async Asset Loading

Load assets without blocking the main thread.

### Usage

```cpp
#include "limbo/assets/AssetLoader.hpp"

// Queue an asset for async loading
AssetLoader::loadAsync<TextureAsset>(assetManager, "textures/player.png",
    [](AssetId id, bool success) {
        if (success) {
            // Asset is ready to use
        }
    });

// Check loading status
if (AssetLoader::isLoading()) {
    // Show loading indicator...
}

// Wait for all assets (blocks)
AssetLoader::waitAll();
```

### Loading Pipeline

1. **Queued**: Request received, waiting for worker thread
2. **LoadingIO**: File I/O and decoding on worker thread
3. **LoadingGPU**: GPU upload on main thread
4. **Loaded**: Ready to use

## SpriteRenderSystem Optimization

The sprite render system uses a dirty flag pattern to avoid sorting every frame.

### Automatic Dirty Detection

The system automatically detects when re-sorting is needed:
- SpriteRendererComponent added/removed
- TransformComponent added/removed

### Manual Dirty Flag

If you modify sorting properties directly, call `markDirty()`:

```cpp
auto& sprite = world.getComponent<SpriteRendererComponent>(entity);
sprite.sortingLayer = 5;
sprite.sortingOrder = 10;

// Tell the system to re-sort
spriteRenderSystem.markDirty();
```

## Performance Tips

1. **Use the profiler** to identify bottlenecks before optimizing
2. **Prefer FrameVector** over std::vector for temporary per-frame data
3. **Batch draw calls** using Renderer2D's batching capabilities
4. **Load assets asynchronously** to avoid frame hitches
5. **Use the thread pool** for CPU-heavy background work
6. **Monitor frame allocator usage** to ensure capacity is sufficient

## Build Configuration

Profiling macros are enabled in Debug builds. To enable in Release:

```cmake
target_compile_definitions(your_target PRIVATE LIMBO_PROFILING_ENABLED=1)
```
