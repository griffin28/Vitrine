#include "VulkanMainWindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QIcon>
#include <QApplication>
#include <QMessageBox>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
VulkanMainWindow::VulkanMainWindow(QWidget *parent, QVulkanWindow *vulkanWindow) 
    : QMainWindow(parent), m_vulkanWindow(vulkanWindow) 
{
    this->setCentralWidget(QWidget::createWindowContainer(m_vulkanWindow, this));
    this->setWindowTitle("Vulkan Sandbox");   
    this->createActions();
    this->createFileMenu();
    this->createHelpMenu();
}

//----------------------------------------------------------------------------------
void VulkanMainWindow::createActions() 
{
    // Exit the application
    m_closeAction = new QAction(QIcon(":/images/power.png"), tr("&Exit"), this);
    connect(m_closeAction, &QAction::triggered, qApp, &QApplication::quit);

    m_aboutAction = new QAction(tr("&About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &VulkanMainWindow::showAboutDialog);

    m_aboutQtAction = new QAction(tr("About &Qt"), this);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

//----------------------------------------------------------------------------------
void VulkanMainWindow::createFileMenu() 
{
    m_fileMenu = this->menuBar()->addMenu(tr("&File"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAction);
}

//----------------------------------------------------------------------------------
void VulkanMainWindow::createHelpMenu() 
{
    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
    m_helpMenu->addAction(m_aboutQtAction);
}

//----------------------------------------------------------------------------------
void VulkanMainWindow::showAboutDialog() 
{
    QMessageBox::about(this, 
                       tr("About Vulkan Sandbox"),
                       tr("<h2>Vulkan Sandbox</h2>"
                          "A Vulkan application using Qt and C++20.<br>"
                          "<p>Copyright &copy; 2026 Dr. Kevin S. Griffin kevin.s.griffin@gmail.com"));
}
} // namespace myvulkan
