#include "AppMainWindow.h"
#include "AppUtils.h"
#include "VulkanWindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QIcon>
#include <QApplication>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QVector>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
AppMainWindow::AppMainWindow(QVulkanInstance* vulkanInstance, QString vulkanInstanceLogMessage, int gpuIndex, QWidget *parent) 
: QMainWindow(parent)
, m_vulkanInstance(vulkanInstance)
, m_selectedGpuIndex(gpuIndex)
{
    this->setWindowTitle("Vulkan Sandbox");
    auto vulkanWindowCreateLogMessage = this->createVulkanWindow();
    this->createCentralWidget();

    this->createActions();
    this->createFileMenu();
    this->createEditMenu();
    this->createHelpMenu();

    // Update 
    if(!vulkanInstanceLogMessage.isEmpty()) 
    {
        this->appendInfoLogMessage("=========================");
        this->appendInfoLogMessage("Vulkan Instance Creation: ");
        this->appendInfoLogMessage("=========================");
        this->appendInfoLogMessage(vulkanInstanceLogMessage.append("\n"));
    }
    this->logSelectedGpuInfo();
    this->appendWarningLogMessage(vulkanWindowCreateLogMessage);
}

//----------------------------------------------------------------------------------
QString AppMainWindow::createVulkanWindow()
{    
    QString warnLogMessage;

    m_vulkanWindow = new VulkanWindow();
    m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

    auto availableDevices = m_vulkanWindow->availablePhysicalDevices();
    if(availableDevices.isEmpty()) 
    {
        qDebug() << "No Vulkan-compatible physical devices found.";
        throw std::runtime_error("No Vulkan-compatible physical devices found.");
    }

    if (m_selectedGpuIndex >= 0 && m_selectedGpuIndex < availableDevices.size()) 
    {
        m_vulkanWindow->setPhysicalDeviceIndex(m_selectedGpuIndex);
    }
    else 
    {
        if(m_selectedGpuIndex >= availableDevices.size()) 
        {
            warnLogMessage.append(tr("Specified GPU index %1 is out of range. There are only %2 available devices. Falling back to automatic selection.")
                              .arg(m_selectedGpuIndex)
                              .arg(availableDevices.size()));
        }
        
        m_selectedGpuIndex = AppUtils::pickPhysicalDevice(availableDevices);
        m_vulkanWindow->setPhysicalDeviceIndex(m_selectedGpuIndex);
    }

    return warnLogMessage;
}

//----------------------------------------------------------------------------------
void AppMainWindow::appendLogMessage(const QString& message, LogLevel level)
{
    if (!m_logWidget) {
        return;
    }

    m_logWidget->appendLogMessage(message, level);
}

//----------------------------------------------------------------------------------
void AppMainWindow::createCentralWidget()
{
    // Vulkan rendering area
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    auto* vulkanContainer = QWidget::createWindowContainer(m_vulkanWindow, m_centralWidget);
    vulkanContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mainLayout->addWidget(vulkanContainer, 1);

    // Log panel
    this->m_logWidget = new CollapsibleLogWidget("Log", 200, m_centralWidget);
    m_mainLayout->addWidget(m_logWidget, 0);

    this->setCentralWidget(m_centralWidget);
}

//----------------------------------------------------------------------------------
void AppMainWindow::createActions() 
{
    // Exit the application
    m_closeAction = new QAction(QIcon(":/images/power.png"), tr("&Exit"), this);
    connect(m_closeAction, &QAction::triggered, qApp, &QApplication::quit);

    m_aboutAction = new QAction(tr("&About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &AppMainWindow::showAboutDialog);

    m_aboutQtAction = new QAction(tr("About &Qt"), this);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);

    m_vulkanPropertiesAction = new QAction(tr("Vulkan &Properties"), this);
    connect(m_vulkanPropertiesAction, &QAction::triggered, this, &AppMainWindow::showVulkanPropertiesDialog);
    
    // Edit menu actions
    m_preferencesAction = new QAction(tr("&Preferences..."), this);
    // m_preferencesAction = new QAction(QIcon(":/images/preferences.png"), tr("&Preferences..."), this);
    connect(m_preferencesAction, &QAction::triggered, this, &AppMainWindow::showPreferencesDialog);
}

//----------------------------------------------------------------------------------
void AppMainWindow::createFileMenu() 
{
    m_fileMenu = this->menuBar()->addMenu(tr("&File"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAction);
}

//----------------------------------------------------------------------------------
void AppMainWindow::createHelpMenu() 
{
    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
    m_helpMenu->addAction(m_aboutQtAction);
    m_helpMenu->addAction(m_vulkanPropertiesAction);
}

//----------------------------------------------------------------------------------
void AppMainWindow::createEditMenu() 
{
    m_editMenu = this->menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_preferencesAction);
}

//----------------------------------------------------------------------------------
void AppMainWindow::showAboutDialog() 
{
    QMessageBox::about(this, 
                       tr("About Vulkan Sandbox"),
                       tr("<h2>Vulkan Sandbox</h2>"
                          "A Vulkan application using Qt and C++20.<br>"
                          "<p>Copyright &copy; 2026 Dr. Kevin S. Griffin kevin.s.griffin@gmail.com"));
}

//----------------------------------------------------------------------------------
void AppMainWindow::showVulkanPropertiesDialog() 
{
    if(m_vulkanPropertiesDialog != nullptr)
    {
        m_vulkanPropertiesDialog->show();
    }
    else 
    {
        if (!m_vulkanWindow) {
            QMessageBox::warning(this, tr("Vulkan Properties"), tr("Vulkan window is not available."));
            return;
        }

        auto instance = m_vulkanWindow->vulkanInstance();

        if (!instance || !instance->isValid()) {
            QMessageBox::warning(this, tr("Vulkan Properties"), tr("Vulkan instance is not valid."));
            return;
        }

        QStringList vulkanProperties;
        vulkanProperties << QString(tr("<b>Vulkan API Version:</b> %1")).arg(instance->supportedApiVersion().toString());

        auto extensions = instance->supportedExtensions();
        vulkanProperties << QString(tr("<br><b>Supported Extensions:</b>"));
        vulkanProperties << QString("<ul>");
        for (const auto& ext : extensions) {
            vulkanProperties << QString("<li>%1</li> ").arg(QString::fromUtf8(ext.name));
        }
        vulkanProperties << QString("</ul>");

        // Create dialog box
        m_vulkanPropertiesDialog = new QMessageBox(this);
        m_vulkanPropertiesDialog->setWindowTitle(tr("Vulkan Properties"));
        m_vulkanPropertiesDialog->setIcon(QMessageBox::Information);
        m_vulkanPropertiesDialog->setStandardButtons(QMessageBox::Ok);

        // create scrollabe view for read-only text
        auto* viewer = new QTextEdit(m_vulkanPropertiesDialog);
        viewer->setReadOnly(true);
        viewer->setHtml(vulkanProperties.join("\n"));
        viewer->setMinimumSize(400, 400);
        viewer->setLineWrapMode(QTextEdit::NoWrap);

        auto* grid = qobject_cast<QGridLayout*>(m_vulkanPropertiesDialog->layout());
        if (grid) {
            const int row = 0;
            const int col = 0;
            const int rowSpan = 1;
            const int colSpan = grid->columnCount() > 0 ? grid->columnCount() : 2;
            grid->addWidget(viewer, row, col, rowSpan, colSpan);
        }

        m_vulkanPropertiesDialog->show();
    }
}

//----------------------------------------------------------------------------------
void AppMainWindow::showPreferencesDialog()
{
    if (!m_vulkanWindow) {
        QMessageBox::warning(this, tr("Preferences"), tr("Vulkan window is not available."));
        return;
    }

    if (!m_preferencesDialog) 
    {
        m_preferencesDialog = new QDialog(this);
        m_preferencesDialog->setWindowTitle(tr("Preferences"));
        m_preferencesDialog->setModal(true);

        auto* mainLayout = new QVBoxLayout(m_preferencesDialog);
        auto* formLayout = new QFormLayout();

        auto* gpuComboBox = new QComboBox(m_preferencesDialog);
        gpuComboBox->setMinimumWidth(320);

        auto availableDevices = m_vulkanWindow->availablePhysicalDevices();
        for(int i=0; i < availableDevices.size(); ++i) {
            const auto& device = availableDevices[i];
            const QString name = QString::fromUtf8(device.deviceName);
            gpuComboBox->addItem(name, i);
        }
        // gpuComboBox->setCurrentIndex(m_selectedGpuIndex);
        formLayout->addRow(new QLabel(tr("Preferred GPU:"), m_preferencesDialog), gpuComboBox);

        mainLayout->addLayout(formLayout);

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                               Qt::Horizontal,
                                               m_preferencesDialog);
        connect(buttonBox, &QDialogButtonBox::accepted, m_preferencesDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, m_preferencesDialog, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        connect(m_preferencesDialog, &QDialog::accepted, this, [this, gpuComboBox]() {
            if (!gpuComboBox) {
                return;
            }

            // m_selectedGpuIndex = gpuComboBox->currentIndex();
            // m_vulkanWindow->setPhysicalDeviceIndex(m_selectedGpuIndex);
            // this->logSelectedGpuInfo();
        });
    }

    m_preferencesDialog->show();
}

//----------------------------------------------------------------------------------
void AppMainWindow::logSelectedGpuInfo() 
{
    if(!m_vulkanWindow) 
    {
        appendErrorLogMessage("Vulkan window is not available.");
        return;
    }

    const auto& device = m_vulkanWindow->physicalDeviceProperties();
    
    if(device == nullptr) 
    {
        appendErrorLogMessage("Failed to retrieve physical device properties.");
        return;
    }

    appendInfoLogMessage("===========================");
    appendInfoLogMessage(QString("Selected GPU Information: %1").arg(m_selectedGpuIndex));
    appendInfoLogMessage("===========================");
    appendInfoLogMessage(QString("%1").arg(device->deviceName));
    appendInfoLogMessage(QString("Vulkan API Version: %1.%2.%3")
            .arg(VK_VERSION_MAJOR(device->apiVersion))
            .arg(VK_VERSION_MINOR(device->apiVersion))
            .arg(VK_VERSION_PATCH(device->apiVersion)));
    appendInfoLogMessage(QString("Driver Version: %1.%2.%3")
            .arg(VK_VERSION_MAJOR(device->driverVersion))
            .arg(VK_VERSION_MINOR(device->driverVersion))
            .arg(VK_VERSION_PATCH(device->driverVersion)));
    appendInfoLogMessage(QString("Vendor ID: %1").arg(device->vendorID));
    appendInfoLogMessage(QString("Device ID: %1\n").arg(device->deviceID));
}
} // namespace myvulkan
