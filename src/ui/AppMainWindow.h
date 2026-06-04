#pragma once

#include <QMainWindow>
#include <QString>
#include <QStyle>
#include <QVector>

#include "AnariBackendDialog.h"
#include "CameraConfig.h"
#include "CollapsibleLogWidget.h"
#include "UserPreferences.h"

class QMenu;
class QAction;
class QMessageBox;
class QVBoxLayout;
class QDialog;
class QIcon;

namespace vitrine
{

class AnariFrameWidget;
class AnariRenderer;
class SceneAxisOverlay;

/// @class AppMainWindow
/// @brief Main application window.
///
/// AppMainWindow owns the GUI-thread AnariRenderer, embeds the
/// AnariFrameWidget as the central widget, and wires menus for opening files,
/// configuring renderer/camera settings, and viewing the log panel.
///
/// The window persists backend, renderer parameter, camera, and user preference
/// state through QSettings and reapplies that state when the ANARI backend is
/// loaded.
class AppMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param anariLibrary CLI override (--anari-library); empty falls back to QSettings
    /// @param darkMode whether the app is currently in dark mode
    /// @param parent parent widget
    explicit AppMainWindow(const QString& anariLibrary,
                           bool darkMode = false,
                           QWidget* parent = nullptr);

    /// @brief Destructor
    ~AppMainWindow() override;

    /// @brief Append an informational message to the log panel
    /// @param message message text to append
    void appendInfoLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Info); }

    /// @brief Append a warning message to the log panel
    /// @param message message text to append
    void appendWarningLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Warning); }

    /// @brief Append an error message to the log panel
    /// @param message message text to append
    void appendErrorLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Error); }

    /// @brief Handle window close by stopping the backend and saving settings
    /// @param event close event to pass to the base class
    void closeEvent(QCloseEvent* event) override;

    /// @brief QSettings group used by the main window
    static constexpr const char* KGROUP = "AppMainWindow";

    /// @brief QSettings key for the selected ANARI library
    static constexpr const char* KANARILIB = "anari/backend/library";

    /// @brief QSettings key for the selected ANARI device subtype
    static constexpr const char* KANARIDEVICESUBTYPE = "anari/backend/deviceSubtype";

    /// @brief QSettings key for the selected ANARI renderer subtype
    static constexpr const char* KANARIRENDERERSUBTYPE = "anari/backend/rendererSubtype";

    /// @brief QSettings key for persisted renderer parameters
    static constexpr const char* KANARIPARAMS = "anari/backend/parameters";

    /// @brief QSettings group for persisted camera configuration
    static constexpr const char* KCAMERA = "camera";

    /// @brief QSettings group for persisted user preferences
    static constexpr const char* KPREFS = "preferences";

private slots:
    /// @brief Prompt the user for a scene file and load it into the renderer
    void openFile();

    /// @brief Show the application about dialog
    void showAboutDialog();

    /// @brief Show the user preferences dialog
    void showPreferencesDialog();

    /// @brief Show the ANARI backend and renderer options dialog
    void showRenderingOptionsDialog();

    /// @brief Show the camera options dialog
    void showCameraOptionsDialog();

    /// @brief Forward renderer status messages to the log panel
    /// @param level log level emitted by the renderer
    /// @param message message text emitted by the renderer
    void onRendererStatusMessage(int level, const QString& message);

    /// @brief Finish backend startup after the renderer reports load completion
    /// @param ok whether the backend loaded successfully
    /// @param libraryName resolved ANARI library name
    /// @param deviceSubtype resolved ANARI device subtype
    void onBackendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype);

    /// @brief Apply configuration selected in the ANARI backend dialog
    /// @param library selected ANARI library name
    /// @param deviceSubtype selected ANARI device subtype
    /// @param rendererSubtype selected ANARI renderer subtype
    /// @param parameters selected renderer parameter values
    void onBackendDialogConfigurationChanged(const QString& library,
                                             const QString& deviceSubtype,
                                             const QString& rendererSubtype,
                                             const QVector<AnariBackendDialog::ParamValue>& parameters);

    /// @brief Apply a camera configuration selected in the camera dialog
    /// @param config selected camera configuration
    void onCameraConfigChanged(const CameraConfig& config);

    /// @brief Apply user preferences selected in the preferences dialog
    /// @param preferences selected user preferences
    void onPreferencesChanged(const UserPreferences& preferences);

private:
    /// @brief Load persisted backend, camera, and preference settings
    void loadSettings();

    /// @brief Save backend, camera, and preference settings
    void saveSettings();

    /// @brief Create the central frame, axis overlay, and log widgets
    void createCentralWidget();

    /// @brief Create the renderer and connect frame/camera signals
    void createRenderer();

    /// @brief Create the actions used by the application menus
    void createActions();

    /// @brief Create the File menu
    void createFileMenu();

    /// @brief Create the Help menu
    void createHelpMenu();

    /// @brief Create the Edit menu
    void createEditMenu();

    /// @brief Create the Options menu
    void createOptionsMenu();

    /// @brief Begin loading the configured ANARI backend
    void startBackend();

    /// @brief Apply renderer parameters to the active renderer
    /// @param parameters renderer parameter values to apply
    void applyParameters(const QVector<AnariBackendDialog::ParamValue>& parameters);

    /// @brief Append a message to the log panel
    /// @param message message text to append
    /// @param level log level to use for display
    void appendLogMessage(const QString& message, LogLevel level);

    /// @brief Apply the current preferences to the relevant widgets
    void applyPreferences();

    /// @brief Resolve a menu-item icon
    /// @param resourcePath bundled resource path to prefer
    /// @param themeName platform theme icon name to use as a secondary choice
    /// @param fallback Qt standard pixmap to use as a fallback
    /// @return resolved icon
    QIcon menuIcon(const QString& resourcePath,
                   const QString& themeName,
                   QStyle::StandardPixmap fallback) const;

    /// @brief Whether dark-mode icon variants should be preferred
    bool m_darkMode = false;

    // Configuration (also persisted to QSettings).
    QString m_anariLibrary{QStringLiteral("phenocryst")};
    QString m_anariDeviceSubtype{QStringLiteral("default")};
    QString m_anariRendererSubtype{QStringLiteral("default")};
    QVector<AnariBackendDialog::ParamValue> m_rendererParameters;
    CameraConfig m_cameraConfig;
    UserPreferences m_preferences;

    // Renderer lives on the GUI thread — many ANARI backends embed
    // libraries (embree, CUDA, etc.) that assume a stable "main-thread"
    // caller, so we don't run it on a worker QThread.
    AnariRenderer* m_renderer = nullptr;

    // UI
    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    AnariFrameWidget* m_frameWidget = nullptr;
    SceneAxisOverlay* m_axisOverlay = nullptr;
    CollapsibleLogWidget* m_logWidget = nullptr;

    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;
    QMenu* m_editMenu = nullptr;
    QMenu* m_optionsMenu = nullptr;

    QAction* m_openFileAction = nullptr;
    QAction* m_closeAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_aboutQtAction = nullptr;
    QAction* m_preferencesAction = nullptr;
    QAction* m_renderingOptionsAction = nullptr;
    QAction* m_cameraOptionsAction = nullptr;
};

} // namespace vitrine
