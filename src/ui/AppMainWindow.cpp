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
#include <QSettings>

#include <algorithm>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
AppMainWindow::AppMainWindow(QVulkanInstance* vulkanInstance, QString vulkanInstanceLogMessage, int gpuIndex, bool darkMode, QWidget *parent) 
: QMainWindow(parent)
, m_vulkanInstance(vulkanInstance)
, m_selectedGpuIndex(gpuIndex)
, m_darkMode(darkMode)
{
    this->setWindowTitle("Vulkan Sandbox");

    // Load saved UI settings
    this->loadSettings();

    QString infoLogMessage, warnLogMessage, errorLogMessage;
    createVulkanWindow(infoLogMessage, warnLogMessage, errorLogMessage);
    this->createCentralWidget();

    this->createActions();
    this->createFileMenu();
    this->createEditMenu();
    this->createOptionsMenu();
    this->createHelpMenu();

    // Update Log
    if(!vulkanInstanceLogMessage.isEmpty()) 
    {
        this->appendInfoLogMessage("=========================");
        this->appendInfoLogMessage("Vulkan Instance Creation: ");
        this->appendInfoLogMessage("=========================");
        this->appendInfoLogMessage(vulkanInstanceLogMessage.append("\n"));
    }
    this->logSelectedGpuInfo();
    this->appendInfoLogMessage(infoLogMessage);
    this->appendWarningLogMessage(warnLogMessage);
    this->appendErrorLogMessage(errorLogMessage);
}

//----------------------------------------------------------------------------------
void AppMainWindow::closeEvent(QCloseEvent* event)
{
    this->saveSettings();
    QMainWindow::closeEvent(event);
}

//----------------------------------------------------------------------------------
void AppMainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("AppMainWindow");

    // Multisampling Anti-aliasing
    m_sampleCount = settings.value(QString::fromUtf8(AppMainWindow::KSAMPLECOUNTKEY), VK_SAMPLE_COUNT_1_BIT).toInt();

    settings.endGroup(); // AppMainWindow
}

//----------------------------------------------------------------------------------
void AppMainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("AppMainWindow");

    // Multisampling Anti-aliasing
    settings.setValue(QString::fromUtf8(AppMainWindow::KSAMPLECOUNTKEY), m_sampleCount);

    settings.endGroup(); // AppMainWindow
}

//----------------------------------------------------------------------------------
void AppMainWindow::createVulkanWindow(QString &infoLogMessage, QString &warnLogMessage, QString &errorLogMessage)
{   
    m_vulkanWindow = new VulkanWindow();
    m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

    // Enable device features
    m_vulkanWindow->setEnabledFeaturesModifier([](VkPhysicalDeviceFeatures& features) {
        // Enable sample shading if multisampling is supported by the device. This can improve the quality 
        // of multisampling by allowing the shader to run at a higher frequency than the rasterization samples, 
        // which can help to reduce aliasing artifacts. However, it can also have a performance impact, 
        // so it's important to use it judiciously and test the performance implications on the target hardware.
        features.sampleRateShading = VK_TRUE;

        // Enable anisotropic filtering for better texture quality at oblique angles
        features.samplerAnisotropy = VK_TRUE;
    });

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
        
        m_selectedGpuIndex = AppUtils::pickPhysicalDevice(m_vulkanInstance);
        m_vulkanWindow->setPhysicalDeviceIndex(m_selectedGpuIndex);
    }
    
    const QList<int> supportedSampleCounts = m_vulkanWindow->supportedSampleCounts();
    if (!supportedSampleCounts.isEmpty())
    {
        int selectedSampleCount = m_sampleCount;

        if (!supportedSampleCounts.contains(selectedSampleCount))
        {
            selectedSampleCount = VK_SAMPLE_COUNT_1_BIT;
            for (const int sampleCount : supportedSampleCounts)
            {
                if (sampleCount > selectedSampleCount)
                {
                    selectedSampleCount = sampleCount;
                }
            }
        }

        m_sampleCount = selectedSampleCount;
        m_vulkanWindow->setSampleCount(selectedSampleCount);
    }

    // Set optional device extensions
    auto supportedExtensions = m_vulkanWindow->supportedDeviceExtensions();
    auto requiredExtensions = QByteArrayList{VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                                             VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
    QByteArrayList validatedExtensions;
    QByteArrayList unsupportedExtensions;

    for(const auto& ext : requiredExtensions) 
    {
        if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&ext](const auto& supportedExt) { return std::strcmp(supportedExt.name, ext.constData()) == 0; }) != supportedExtensions.end()) 
        {
            validatedExtensions.append(ext);
        } else {
            unsupportedExtensions.append(ext);
        }
    }

    if(!validatedExtensions.isEmpty()) 
    {
        m_vulkanWindow->setDeviceExtensions(validatedExtensions);
        // TODO: update m_vulkanWindow to support setting device features based on enabled extensions
        infoLogMessage.append(tr("Enabled device extensions: %1").arg(validatedExtensions.join(", ")));
    }

    if(!unsupportedExtensions.isEmpty()) 
    {
        errorLogMessage.append(tr("Required device extensions not supported: %1").arg(unsupportedExtensions.join(", ")));
    }
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
// ":/qdarkstyle/dark/darkstyle.qss"
    const bool isLightTheme = !m_darkMode;
    // const QString iconPrefix = isLightTheme
    //     ? QStringLiteral(":/qss_icons/light")
    //     : QStringLiteral(":/qss_icons/dark");
    const QString iconPrefix = QStringLiteral(":/qdarkstyle/dark");

    m_renderingOptionsAction = new QAction(
        QIcon(iconPrefix + QStringLiteral("rc/toolbar_move_horizontal.png")),
        tr("&Rendering..."), this);
    connect(m_renderingOptionsAction, &QAction::triggered, this, &AppMainWindow::showRenderingOptionsDialog);
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
void AppMainWindow::createOptionsMenu()
{
    m_optionsMenu = this->menuBar()->addMenu(tr("&Options"));
    m_optionsMenu->addAction(m_renderingOptionsAction);
}

//----------------------------------------------------------------------------------
void AppMainWindow::showRenderingOptionsDialog()
{
    if (!m_vulkanWindow)
    {
        QMessageBox::warning(this, tr("Rendering Options"), tr("Vulkan window is not available."));
        return;
    }

    if (!m_renderingOptionsDialog)
    {
        m_renderingOptionsDialog = new QDialog(this);
        m_renderingOptionsDialog->setWindowTitle(tr("Rendering Options"));
        m_renderingOptionsDialog->setModal(true);

        auto* mainLayout = new QVBoxLayout(m_renderingOptionsDialog);
        auto* formLayout = new QFormLayout();

        auto* sampleCountComboBox = new QComboBox(m_renderingOptionsDialog);
        sampleCountComboBox->setMinimumWidth(160);

        QList<int> supportedSampleCounts = m_vulkanWindow->supportedSampleCounts();
        std::sort(supportedSampleCounts.begin(), supportedSampleCounts.end(), std::greater<int>());
        const int currentSampleCount = static_cast<int>(m_vulkanWindow->sampleCountFlagBits());

        for (const int count : supportedSampleCounts)
        {
            sampleCountComboBox->addItem(tr("%1x MSAA").arg(count), count);
            if (count == currentSampleCount)
            {
                sampleCountComboBox->setCurrentIndex(sampleCountComboBox->count() - 1);
            }
        }

        QLabel* msaaLabel = new QLabel(tr("MSAA Sample Count:"), m_renderingOptionsDialog);
        msaaLabel->setToolTip(tr("Multisampling Anti-Aliasing. Changes will take effect after restarting the application."));
        formLayout->addRow(msaaLabel, sampleCountComboBox);
        mainLayout->addLayout(formLayout);

        auto* buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            m_renderingOptionsDialog);
        connect(buttonBox, &QDialogButtonBox::accepted, m_renderingOptionsDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, m_renderingOptionsDialog, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        connect(m_renderingOptionsDialog, &QDialog::accepted, this, [this, sampleCountComboBox]() {
            if (!m_vulkanWindow || !sampleCountComboBox)
            {
                return;
            }
            const int selectedCount = sampleCountComboBox->currentData().toInt();
            const int activeCount = static_cast<int>(m_vulkanWindow->sampleCountFlagBits());

            if (selectedCount != activeCount)
            {
                m_sampleCount = selectedCount;

                if (m_vulkanWindow->isValid())
                {
                    this->appendWarningLogMessage(
                        tr("Multisampling Anti-Aliasing change to %1x MSAA. Restart the app to apply it.")
                            .arg(selectedCount));
                }
                else
                {
                    m_vulkanWindow->setSampleCount(selectedCount);
                    this->appendInfoLogMessage(tr("Rendering sample count set to %1x MSAA").arg(selectedCount));
                }
            }
        });
    }

    m_renderingOptionsDialog->show();
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

        auto deviceProperties = m_vulkanWindow->physicalDeviceProperties();

        QStringList vulkanProperties;
        vulkanProperties << QString(tr("<b>Vulkan API Version:</b> %1")).arg(deviceProperties ? QString("%1.%2.%3")
            .arg(VK_VERSION_MAJOR(deviceProperties->apiVersion))
            .arg(VK_VERSION_MINOR(deviceProperties->apiVersion))
            .arg(VK_VERSION_PATCH(deviceProperties->apiVersion)) : "N/A");

        auto extensions = m_vulkanWindow->supportedDeviceExtensions();
        vulkanProperties << QString(tr("<br><b>Supported Device Extensions:</b>"));
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
