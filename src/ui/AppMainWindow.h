#pragma once

#include <QMainWindow>
#include <QVulkanWindow>

#include "CollapsibleLogWidget.h"

class QMenu;
class QAction;
class QMessageBox;
class QVBoxLayout;
class QDialog;
class QComboBox;


namespace myvulkan 
{
class AppMainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    /// @brief Constructor for AppMainWindow.
    /// @param vulkanInstance the Vulkan instance to use, default is nullptr (will create a new instance).
    /// @param vulkanInstanceLogMessage log messages from the Vulkan instance creation, default is an empty string.
    /// @param gpuIndex the index of the GPU to use, default is -1 (automatic selection).
    /// @param parent the parent widget, default is nullptr.
    AppMainWindow(QVulkanInstance* vulkanInstance,
                     QString vulkanInstanceLogMessage,
                     int gpuIndex = -1,
                     bool darkMode = false,
                     QWidget *parent = nullptr);
    
    /// @brief Destructor for AppMainWindow.
    ~AppMainWindow() = default;

    /// @brief appends an informational log message to the log panel.
    /// @param message the log message to append.
    void appendInfoLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Info); }

    /// @brief appends a warning log message to the log panel.
    /// @param message the log message to append.
    void appendWarningLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Warning); }
    
    /// @brief appends an error log message to the log panel.
    /// @param message the log message to append.
    void appendErrorLogMessage(const QString& message) { appendLogMessage(message, LogLevel::Error); }

    /// @brief Handles the close event for the main window.
    /// @param event the close event.
    void closeEvent(QCloseEvent* event) override;
    
    // Setting Keys
    static constexpr const char* KSAMPLECOUNTKEY = "options/rendering/sampleCount";

private slots:
    void showAboutDialog();
    void showVulkanPropertiesDialog();
    void showPreferencesDialog();
    void showRenderingOptionsDialog();

private:
    void loadSettings();
    void saveSettings();

    void createVulkanWindow(QString& infoLogMessage, QString& warnLogMessage, QString& errorLogMessage);
    void createCentralWidget();
    void createActions();
    void createFileMenu();
    void createHelpMenu();
    void createEditMenu();
    void createOptionsMenu();

    void appendLogMessage(const QString& message, LogLevel level);
    void logSelectedGpuInfo();

    QVulkanWindow* m_vulkanWindow = nullptr;
    QVulkanInstance* m_vulkanInstance = nullptr;
    int m_selectedGpuIndex = -1;
    bool m_darkMode = false;

    // Settings
    int m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    CollapsibleLogWidget* m_logWidget = nullptr;

    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;
    QMenu* m_editMenu = nullptr;
    QMenu* m_optionsMenu = nullptr;

    // File menu actions
    QAction* m_closeAction = nullptr;

    // Help menu actions
    QAction* m_aboutAction = nullptr;
    QAction* m_aboutQtAction = nullptr;
    QAction* m_vulkanPropertiesAction = nullptr;

    // Edit menu actions
    QAction* m_preferencesAction = nullptr;

    // Options menu actions
    QAction* m_renderingOptionsAction = nullptr;

    QMessageBox* m_vulkanPropertiesDialog = nullptr;
    QDialog* m_preferencesDialog = nullptr;
    QDialog* m_renderingOptionsDialog = nullptr;
};
}