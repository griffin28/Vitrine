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

private slots:
    void showAboutDialog();
    void showVulkanPropertiesDialog();
    void showPreferencesDialog();

private:
    void createVulkanWindow(QString& infoLogMessage, QString& warnLogMessage, QString& errorLogMessage);
    void createCentralWidget();
    void createActions();
    void createFileMenu();
    void createHelpMenu();
    void createEditMenu();

    void appendLogMessage(const QString& message, LogLevel level);
    void logSelectedGpuInfo();

    QVulkanWindow* m_vulkanWindow = nullptr;
    QVulkanInstance* m_vulkanInstance = nullptr;
    int m_selectedGpuIndex;

    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    CollapsibleLogWidget* m_logWidget = nullptr;

    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;
    QMenu* m_editMenu = nullptr;

    // File menu actions
    QAction* m_closeAction = nullptr;

    // Help menu actions
    QAction* m_aboutAction = nullptr;
    QAction* m_aboutQtAction = nullptr;
    QAction* m_vulkanPropertiesAction = nullptr;

    // Edit menu actions
    QAction* m_preferencesAction = nullptr;

    QMessageBox* m_vulkanPropertiesDialog = nullptr;
    QDialog* m_preferencesDialog = nullptr;
};
}