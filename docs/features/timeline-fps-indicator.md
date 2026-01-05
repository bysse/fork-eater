# Feature: Timeline FPS Indicator

## 1. Summary

The Timeline FPS Indicator is a visual tool that displays the application's real-time frame rate directly on the timeline bar, providing immediate feedback on rendering performance.

## 2. Core Functionality

-   **Real-time FPS Display**: Calculates and displays the frames per second for each time slice of the timeline.
-   **Color-Coded Performance**: The indicator is color-coded to provide an at-a-glance assessment of performance:
    -   **Green**: FPS is above the high threshold (good performance).
    -   **Yellow/Orange**: FPS is between the low and high thresholds.
    -   **Red**: FPS is below the low threshold, or the render scale has been reduced due to poor performance. This state indicates a performance bottleneck.
    -   **Grey**: No FPS data has been recorded for this time slice.
-   **Hybrid Dynamic Render Scaling**: The system uses a two-phase approach to automatically adjust the render scale to maintain a target frame rate.
    -   **1. Aggressive Drop**: If the instantaneous FPS falls below critical levels (5 or 10 FPS), the render scale is immediately reduced to a moderate level (e.g., 40% or 60%) or even to a very low scale like 5% (0.05f) to provide a rapid recovery.
    -   **2. Progressive Scaling**: For less severe performance fluctuations, the system uses a running average of the last 10 frames to make gradual adjustments. It progressively lowers the render scale if the average FPS is below a target (~35 FPS) and gradually raises it back to 100% if the average FPS is consistently high (above 50 FPS).
-   **Render Scale Awareness**: If the application's render scale is reduced to improve performance, the timeline indicator will turn red, regardless of the resulting frame rate. This ensures that performance issues are not hidden by automatic scaling.

## 3. Key Components & Files

| Component/File             | Type  | Role                                                                                                                              |
| -------------------------- | ----- | --------------------------------------------------------------------------------------------------------------------------------- |
| `Timeline`                 | Class | Manages the timeline's state, including the storage and rendering of FPS data.                                                    |
| `src/Timeline.cpp`         | File  | Implements the logic for storing FPS data (including render scale) and rendering the color-coded graph on the timeline bar.       |
| `include/Timeline.h`       | File  | Defines the `Timeline` class interface and the `FPSData` struct that holds both `fps` and `renderScaleFactor`.                    |
| `ShaderEditor`             | Class | Implements the dynamic render scaling logic, calculates FPS, and reports metrics to the Timeline.     |
| `src/ShaderEditor.cpp`     | File  | Contains the hybrid scaling logic in its `render()` loop, includes a guard against division by near-zero frame times, and uses `glFinish()` to ensure accurate GPU frame time measurement. |
| `include/ShaderEditor.h`   | File  | Contains the `m_fpsHistory` deque for averaging recent frame rates. |
| `Settings`                 | Class | Stores the configuration for the low and high FPS thresholds that determine the color-coding and scaling behavior.                                     |
| `include/Settings.h`       | File  | Defines the default values for `m_lowFPSThreshold` (20.0f) and `m_highFPSThreshold` (50.0f).                                      |

## 4. Configuration

The FPS color thresholds and scaling behavior are configured in the `Settings` class (`include/Settings.h` and `src/Settings.cpp`).

-   `m_lowFPSThreshold`: The FPS value below which the indicator turns red. **Default: 20.0f**.
-   `m_highFPSThreshold`: The FPS value above which the indicator turns green and progressive scaling may begin to recover resolution. **Default: 50.0f**.
-   `m_lowFPSRenderThreshold50` / `25`: The thresholds for the initial aggressive scaling drop. **Defaults: 10.0f / 5.0f**.

These values can also be adjusted in the application's settings file (`~/.config/fork-eater/settings.conf`).

## 5. Usage & Interactions

The FPS indicator is a passive feature that is always active. It automatically displays performance on the timeline bar during playback. It does not require any direct user interaction.

-   **Interaction with Render Scaling**: The indicator's most critical interaction is with the dynamic render scaling feature. When `ShaderEditor` reduces the `renderScaleFactor` due to low FPS, the timeline is explicitly notified. The timeline then forces the indicator to red for that time slice, providing clear feedback that the visual quality has been degraded to maintain performance. The scaling logic is designed to automatically find a balance between visual quality and the target frame rate.
