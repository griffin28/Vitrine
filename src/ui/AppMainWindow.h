#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "AnariBackendDialog.h"
#include "CollapsibleLogWidget.h"

class QMenu;
class QAction;
class QMessageBox;
class QVBoxLayout;
class QDialog;

namespace vitrine
{

class AnariFrameWidget;
class AnariRenderer;

/// @brief Main application window. Owns the AnariRenderer (as a GUI-thread
///        child QObject) and embeds the AnariFrameWidget as the central
///        widget; menus let the user open the backend / parameter dialog
///        and view the log panel.
class AppMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @param anariLibrary  CLI override (--anari-library); empty falls back
    ///                      to QSettings (default: "phenocryst").
    /// @param darkMode      Whether the app is currently in dark mode (used
    ///                      to pick icon variants).
    explicit AppMainWindow(const QString& anariLibrary,
                           bool darkMode = false,
                           QWidget* parent = nullptr);
    ~AppMainWindow() override;

    void appendInfoLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Info); }
    void appendWarningLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Warning); }
    void appendErrorLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Error); }

    void closeEvent(QCloseEvent* event) override;

    // Settings keys
    static constexpr const char* KGROUP = "AppMainWindow";
    static constexpr const char* KANARILIB = "anari/backend/library";
    static constexpr const char* KANARIDEVICESUBTYPE = "anari/backend/deviceSubtype";
    static constexpr const char* KANARIRENDERERSUBTYPE = "anari/backend/rendererSubtype";
    static constexpr const char* KANARIPARAMS = "anari/backend/parameters";

private slots:
    void openFile();
    void showAboutDialog();
    void showPreferencesDialog();
    void showRenderingOptionsDialog();
    void onRendererStatusMessage(int level, const QString& message);
    void onBackendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype);
    void onBackendDialogConfigurationChanged(const QString& library,
                                             const QString& deviceSubtype,
                                             const QString& rendererSubtype,
                                             const QVector<AnariBackendDialog::ParamValue>& parameters);

private:
    void loadSettings();
    void saveSettings();

    void createCentralWidget();
    void createRenderer();
    void createActions();
    void createFileMenu();
    void createHelpMenu();
    void createEditMenu();
    void createOptionsMenu();
    void startBackend();
    void applyParameters(const QVector<AnariBackendDialog::ParamValue>& parameters);

    void appendLogMessage(const QString& message, LogLevel level);

    bool m_darkMode = false;

    // Configuration (also persisted to QSettings).
    QString m_anariLibrary{QStringLiteral("phenocryst")};
    QString m_anariDeviceSubtype{QStringLiteral("default")};
    QString m_anariRendererSubtype{QStringLiteral("default")};
    QVector<AnariBackendDialog::ParamValue> m_rendererParameters;

    // Renderer lives on the GUI thread — many ANARI backends embed
    // libraries (embree, CUDA, etc.) that assume a stable "main-thread"
    // caller, so we don't run it on a worker QThread.
    AnariRenderer* m_renderer = nullptr;

    // UI
    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    AnariFrameWidget* m_frameWidget = nullptr;
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

    QDialog* m_preferencesDialog = nullptr;
};

} // namespace vitrine
