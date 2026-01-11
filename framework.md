### Startup Logic
The program entry point is the `main()` function in `main.cpp` under `src/VCX/Labs/<lab-name>`, which actually calls `Engine::RunApp()`. This function sequentially executes initialization (`Internal::RunApp_Init`), main loop (`Internal::RunApp_Main`), and shutdown (`Internal::RunApp_Shutdown`). Within the main loop, `app.OnFrame()` is invoked through `RunApp_Frame()`, which ultimately executes `UI::Setup()` to build the interface.

### Core of Interface Construction
`UI::Setup()` constructs the interface via `setupSideWindow()` (sidebar) and `setupMainWindow()` (main window), relying critically on three functions:
- `OnSetupPropsUI()`: Configures interactive components (e.g., checkboxes) for the sidebar, supporting custom sidebars for different cases.
- `OnRender()`: Generates content displayed in the main window, supporting 2D images (by constructing `ImageRGB` objects) and 3D visualization (based on OpenGL, loading elements such as points, line segments, and triangles via `RenderItem`).
- `OnProcessInput()`: Handles interaction logic. For 2D scenes, it supports functions like mouse drag scrolling and magnifying glass; for 3D scenes, it enables mouse control over camera rotation, focus movement, zooming, etc.

### II. Development Method
#### Core Steps
Define a new `App` class, and within it, define a `Case` class. Override the three functions: `OnSetupPropsUI()` (sidebar development), `OnRender()` (main window development), and `OnProcessInput()` (interaction development). These three functions are called every frame to achieve the desired effects for the corresponding case.

#### Development Examples for Each Module
- **Sidebar**: Create interactive components and bind variables through ImGui interfaces (e.g., `ImGui::Checkbox`).
- **Main Window**: For 2D scenes, fixed-size or resizable images can be generated. For 3D scenes, basic elements such as points and line segments (requiring specified `Indices` to indicate connection order) can be loaded via `RenderItem` to construct complex graphics.
- **Interaction**: For 2D scenes, mouse scrolling and magnifying glass functions can be implemented. For 3D scenes, camera control can be achieved through `_cameraManager.ProcessInput()`.