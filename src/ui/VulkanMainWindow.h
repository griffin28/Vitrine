#pragma once

#include "CollapsibleLogWidget.h"

#include <QMainWindow>
#include <QVulkanWindow>

class QMenu;
class QAction;
class QMessageBox;
class QVBoxLayout;
class QDialog;
class QComboBox;

namespace myvulkan 
{
class VulkanMainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    /// @brief Constructor for VulkanMainWindow.
    /// @param parent the parent widget, default is nullptr.
    /// @param vulkanWindow the QVulkanWindow instance to be used for rendering, default is nullptr.
    VulkanMainWindow(QWidget *parent = nullptr, QVulkanWindow *vulkanWindow = nullptr);
    
    /// @brief Destructor for VulkanMainWindow.
    ~VulkanMainWindow() = default;

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
    void createCentralWidget();
    void createActions();
    void createFileMenu();
    void createHelpMenu();
    void createEditMenu();

    void appendLogMessage(const QString& message, LogLevel level);
    void logSelectedGpuInfo();

    QVulkanWindow* m_vulkanWindow = nullptr;

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