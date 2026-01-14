<div style="text-align: center;">
    <h1>VCL Final Project Report: BVH Loader</h1>
    <p style="font-style: italic; font-size: 20px">王一楷&emsp;2400013557</p>
    <p style="font-style: italic; font-size: 20px">2026/1/10</p>
</div>

---

## 1. Introduction
This project implements a complete BVH (BioVision Hierarchy) file loader and skeleton animation rendering system, designed to parse BVH motion capture files, construct 3D skeleton structures, and render real-time skeleton animations with interactive controls. The system supports core functionalities such as dynamic BVH file selection, skeleton visualization, animation playback control, anti-aliasing adjustment, and video frame export, providing an intuitive interface for viewing and manipulating skeletal motion data.

---

## 2. System Architecture
The project is modularized into distinct components, each responsible for a specific task. The key modules and their interactions are outlined below:

### 2.1 Module Breakdown
| Module               | File(s)               | Responsibility                                                                 |
|----------------------|-----------------------|---------------------------------------------------------------------------------|
| Skeleton Model       | `Skeleton.h/cpp`      | Defines `Joint` and `Skeleton` structures; implements forward kinematics, memory management (clear), and conversion to renderable data. |
| BVH Parser/Loader    | `BVHLoader.h/cpp`     | Reads BVH files, parses hierarchical joint data (HIERARCHY section) and motion frames (MOTION section); constructs the skeleton and populates animation data. |
| Animation Player     | `Player.h/cpp`        | Manages animation playback (frame progression, reset, applying motion data to the skeleton); drives forward kinematics updates. |
| Rendering            | `CaseSkeleton.h/cpp`, `CaseBVH.h/cpp`       | Implements 3D rendering; handles UI controls and camera interaction. |
| Application Core     | `App.h/cpp`, `main.cpp` | Initializes the application, sets up the UI framework, and runs the main render loop. |

### 2.2 Data Flow
1. **BVH File Loading**: The `BVHLoader` reads a BVH file, splitting the content into the `HIERARCHY` (skeleton structure) and `MOTION` (animation frames) sections.
2. **Skeleton Construction**: `BVHLoader::ConstructTree()` builds a hierarchical tree of `Joint` objects, populating each joint’s local offset, rotation/position indices, and child/sibling pointers.
3. **Animation Data Storage**: `BVHLoader::ConstructAction()` parses motion frames (frame count, frame time, joint parameters) and stores them in an `Action` object.
4. **Animation Playback**: The `Action` class updates the skeleton’s joint rotations/offsets per frame, triggering `Skeleton::ForwardKinematics()` to compute global joint positions/rotations.
5. **Rendering**: The `SkeletonRender` class converts the skeleton’s joint data into renderable vertices/indices, and the `CaseBVH` class renders the skeleton (lines for bones, points for joints) and a background floor using OpenGL.

---

## 3. Key Technical Implementations
### 3.1 Skeletal Hierarchy Representation
The skeleton is represented as a tree of `Joint` objects, with each joint storing:
- **Spatial Data**: Local offset (relative to parent), local/global rotation (quaternions), and global position (vec3).
- **Tree Pointers**: `BroPtr` (sibling joint) and `ChiPtr` (first child joint) to form the hierarchical structure.
- **Index Mapping**: `PositionIdx` and `RotationIdx` to map BVH channel data (X/Y/Z rotation/position) to joint properties.

The `Skeleton` class manages the root joint, with methods to:
- Clear the skeleton (recursive deletion of joints to avoid memory leaks).
- Convert the skeleton to renderable data (`Convert()`: vertices for joints, indices for bone lines).
- Execute forward kinematics (`ForwardKinematics()`):
  1. Initialize the root joint’s global position/rotation.
  2. Recursively compute child joints’ global transforms (`ItsMyGo()`: `global_rotation = parent_rotation * local_rotation`; `global_position = parent_position + parent_rotation * local_offset`).
  3. Adjust joint positions with scale/offset (`Adjust()` for scene alignment).

### 3.2 BVH File Parsing
The `BVHLoader` class parses BVH files with two core methods:
- `ConstructTree()`: Recursively parses the `HIERARCHY` section to build the joint tree. It handles:
  - Root joint initialization.
  - Joint offset extraction.
  - Channel mapping (X/Y/Z rotation/position to joint indices).
  - End sites (terminal joints with no further children).
- `ConstructAction()`: Parses the `MOTION` section to extract frame count, frame time, and per-frame joint parameters (stored in `Action::FrameParams` as a 2D vector of floats).

Helper methods:
- `split()`: Splits a line of text into tokens (handles whitespace/tabs).
- `GetLine()`: Reads a line from the file and splits it into tokens, skipping empty lines.

### 3.3 Animation Playback
The `Action` class manages animation playback:
- `Load()`: Advances the animation by `dt` (delta time), calculates the current frame, and applies the frame’s joint parameters to the skeleton.
- `Play()`: Recursively updates joint local rotations/offsets from frame parameters, converting Euler angles (from BVH) to quaternions for rotation.
- `Reset()`: Resets the animation to the first frame.

### 3.4 Rendering
- **Background**: A large gray floor (2 triangles) rendered as a static 3D object.
- **Skeleton**: 
  - Joints: Rendered as red points (OpenGL `GL_POINTS`).
  - Bones: Rendered as white lines (OpenGL `GL_LINES`) connecting parent/child joints.
- **Shader**: Uses a simple flat shader (`flat.vert`/`flat.frag`) for unlit rendering, with uniform variables for projection/view matrices and color.

---

## 4. Result
Run the following command lines in Powershell at the root directory `VCL-Final-Project`. The bvh files are saved in `assets/BVH_data`.
```
xmake
xmake run final
```
In this way you can see the UI as `UI1.png` and `UI2.png` show.

There are two cases in the project. `Case 1: Skeleton Structure` shows a static skeleton, where the user can **hover your mouse cursor over a joint to see its index and name in the sidebar**. The main purpose of this case is to help user check whether the skeleton structure is consistent in different bvh files to avoid matching error in further works such as skinning. `Case 2: BVH Animation` renders a complete skeleton animation from bvh files, where the user can **control the playing speed**, **play/pause/reset** the animation, and **export frames** to a folder in `build/windows/x64/release` (it's a pity that I failed to directly export a video, which typicallly requires `FFmpeg` that isn't included in the project's structure. The user can convert these frames to video using `FFmpeg` later, though. Besides, it's normal to have a lower framerate when exporting frames). I also include some useful functions in both cases including **file selection**, **anti-aliasing** and **camera control** (there's a note in the sidebar on how to use it).