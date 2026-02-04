#pragma once

#include <QMainWindow>
#include <QVulkanWindow>

class QMenu;
class QAction;
class QMessageBox;

namespace myvulkan 
{
class VulkanMainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    VulkanMainWindow(QWidget *parent = nullptr, QVulkanWindow *vulkanWindow = nullptr);
    ~VulkanMainWindow() = default;

private slots:
    void showAboutDialog();
    void showVulkanPropertiesDialog();

private:
    void createActions();
    void createFileMenu();
    void createHelpMenu();

    QVulkanWindow* m_vulkanWindow = nullptr;

    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;

    QAction* m_closeAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_aboutQtAction = nullptr;
    QAction* m_vulkanPropertiesAction = nullptr;

    QMessageBox* m_vulkanPropertiesDialog = nullptr;
};
}