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
-   **Render Scale Awareness**: If the application's render scale is reduced (to 50% or 25%) to improve performance, the timeline indicator will turn red, regardless of the resulting frame rate. This ensures that performance issues are not hidden by the automatic scaling.

## 3. Key Components & Files

| Component/File             | Type  | Role                                                                                                                              |
| -------------------------- | ----- | --------------------------------------------------------------------------------------------------------------------------------- |
| `Timeline`                 | Class | Manages the timeline's state, including the storage and rendering of FPS data.                                                    |
| `src/Timeline.cpp`         | File  | Implements the logic for storing FPS data (including render scale) and rendering the color-coded graph on the timeline bar.       |
| `include/Timeline.h`       | File  | Defines the `Timeline` class interface and the `FPSData` struct that holds both `fps` and `renderScaleFactor`.                    |
| `ShaderEditor`             | Class | Calculates the FPS after rendering a shader pass and reports it, along with the current render scale factor, to the Timeline.     |
| `src/ShaderEditor.cpp`     | File  | Contains the primary FPS calculation logic within its `render()` loop and includes a guard against division by near-zero frame times. |
| `Settings`                 | Class | Stores the configuration for the low and high FPS thresholds that determine the color-coding.                                     |
| `include/Settings.h`       | File  | Defines the default values for `m_lowFPSThreshold` (20.0f) and `m_highFPSThreshold` (50.0f).                                      |

## 4. Configuration

The FPS color thresholds are configured in the `Settings` class (`include/Settings.h` and `src/Settings.cpp`).

-   `m_lowFPSThreshold`: The FPS value below which the indicator turns red. **Default: 20.0f**.
-   `m_highFPSThreshold`: The FPS value above which the indicator turns green. **Default: 50.0f**.

These values can also be adjusted in the application's settings file (`~/.config/fork-eater/settings.conf`).

## 5. Usage & Interactions

The FPS indicator is a passive feature that is always active. It automatically displays performance on the timeline bar during playback. It does not require any direct user interaction.

-   **Interaction with Render Scaling**: The indicator's most critical interaction is with the dynamic render scaling feature. When `ShaderEditor` reduces the `renderScaleFactor` due to low FPS, the timeline is explicitly notified. The timeline then forces the indicator to red for that time slice, providing clear feedback that the visual quality has been degraded to maintain performance.
