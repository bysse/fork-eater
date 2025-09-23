# DPI Scaling and UI Scaling

Fork Eater now includes comprehensive DPI awareness and UI scaling support to ensure the application looks crisp and appropriately sized on high-DPI displays.

## Features

### Automatic DPI Detection
- **Auto-detect mode**: Automatically detects your monitor's DPI scaling and applies appropriate scaling factors
- **Multiple detection methods**: Uses GLFW content scaling (preferred) and falls back to physical monitor size calculations
- **Smart scaling factors**: Rounds to common scaling factors (1.0x, 1.25x, 1.5x, 2.0x, 2.5x, 3.0x) for best results

### Manual Scaling Control
- **Custom UI Scale**: Adjust UI element sizes (buttons, panels, spacing) from 0.5x to 4.0x
- **Custom Font Scale**: Independently control font scaling from 0.5x to 4.0x
- **Real-time updates**: Changes apply immediately without requiring a restart

### Settings Persistence
- **Configuration file**: Settings are automatically saved to `~/.config/fork-eater/settings.conf`
- **Cross-session**: Your scaling preferences are remembered between application launches

## Usage

### GUI Settings
1. Open **Options → Settings...** from the menu bar
2. Choose your DPI scale mode:
   - **Auto-detect**: Automatically detect and apply system DPI scaling
   - **Manual**: Set custom scaling factors
   - **Disabled**: Use 1.0x scaling (no scaling)
3. If using Manual mode, adjust the UI Scale and Font Scale sliders
4. Settings are automatically saved

### Command Line Options

Override DPI scaling from the command line:

```bash
# Use custom 1.5x scaling
./fork-eater --scale 1.5 shader_project/

# Disable DPI scaling completely
./fork-eater --no-dpi-scale shader_project/

# See all options
./fork-eater --help
```

## Technical Details

### DPI Detection Methods
1. **GLFW Content Scale** (preferred): Uses `glfwGetMonitorContentScale()` for accurate scaling on supported platforms
2. **Physical Size Calculation**: Calculates DPI from monitor resolution and physical dimensions when content scale is unavailable

### Scaling Application
- **Window Size**: Initial window size is scaled based on detected DPI
- **ImGui Font Scaling**: Uses `ImGuiIO::FontGlobalScale` for font scaling
- **ImGui Style Scaling**: Scales padding, spacing, scrollbar size, and other UI elements
- **Real-time Updates**: Settings changes apply immediately to the ImGui context

### Configuration File Format
```ini
# Fork Eater Settings
# DPI scale mode: auto, manual, disabled
dpi_scale_mode=auto
ui_scale_factor=1.50
font_scale_factor=1.50
```

## Troubleshooting

### Application appears too small on high-DPI displays
- Try enabling **Auto-detect** mode in Settings
- If auto-detection doesn't work well, switch to **Manual** mode and adjust the scaling factors
- Use the command line option `--scale 2.0` to force 2x scaling

### Application appears too large
- Switch to **Manual** mode and reduce the scaling factors
- Use `--scale 0.75` to force smaller scaling
- Use `--no-dpi-scale` to disable scaling entirely

### Settings not saved
- Ensure the configuration directory `~/.config/fork-eater/` is writable
- Check the console output for any error messages about settings file access

## Platform Notes

### Linux
- Works best with Wayland and X11
- GLFW content scale detection typically works well on modern desktop environments
- Falls back to physical size calculation on older systems

### Other Platforms
- DPI detection methods may vary by platform
- Manual scaling mode provides consistent behavior across all platforms
- Physical size calculation provides reasonable fallback behavior

## Best Practices

1. **Start with Auto-detect**: Let Fork Eater detect your system DPI automatically
2. **Fine-tune if needed**: Switch to Manual mode if auto-detection doesn't match your preferences
3. **Use common factors**: Stick to common scaling factors (1.0x, 1.25x, 1.5x, 2.0x) for best visual results
4. **Independent scaling**: Use different font and UI scaling if you prefer larger text with normal UI elements