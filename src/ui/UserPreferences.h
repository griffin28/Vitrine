#pragma once

namespace vitrine
{

/// @brief User-facing application preferences, edited via PreferencesDialog and
///        persisted through QSettings. Intentionally a plain growable struct:
///        adding a preference is a one-line addition here plus a widget in the
///        dialog and a load/save line in AppMainWindow.
struct UserPreferences
{
    /// @brief Whether the camera axis-overlay gizmo is shown over the frame.
    bool showAxisOverlay{true};
};

} // namespace vitrine
