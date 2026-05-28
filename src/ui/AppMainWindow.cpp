#include "AppMainWindow.h"

#include "AnariFrameWidget.h"
#include "AnariRenderer.h"
#include "AnariUtils.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

namespace vitrine
{

namespace
{
constexpr int kDefaultLogHeight = 200;
}

AppMainWindow::AppMainWindow(const QString& anariLibrary, bool darkMode, QWidget* parent)
    : QMainWindow(parent), m_darkMode(darkMode)
{
    setWindowTitle(tr("Vitrine"));

    loadSettings();
    if (!anariLibrary.isEmpty()) {
        m_anariLibrary = anariLibrary;
    }

    createCentralWidget();
    createRenderer();

    createActions();
    createFileMenu();
    createEditMenu();
    createOptionsMenu();
    createHelpMenu();

    startBackend();
}

AppMainWindow::~AppMainWindow()
{
    if (m_renderer) {
        m_renderer->stop();
    }
}

void AppMainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void AppMainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup(KGROUP);
    m_anariLibrary = settings.value(KANARILIB, m_anariLibrary).toString();
    m_anariDeviceSubtype = settings.value(KANARIDEVICESUBTYPE, m_anariDeviceSubtype).toString();
    m_anariRendererSubtype = settings.value(KANARIRENDERERSUBTYPE, m_anariRendererSubtype).toString();

    m_rendererParameters.clear();
    const int count = settings.beginReadArray(KANARIPARAMS);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        AnariBackendDialog::ParamValue p;
        p.name = settings.value("name").toString();
        p.type = settings.value("type").toInt();
        p.value = settings.value("value");
        if (!p.name.isEmpty()) {
            m_rendererParameters.push_back(p);
        }
    }
    settings.endArray();
    settings.endGroup();
}

void AppMainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup(KGROUP);
    settings.setValue(KANARILIB, m_anariLibrary);
    settings.setValue(KANARIDEVICESUBTYPE, m_anariDeviceSubtype);
    settings.setValue(KANARIRENDERERSUBTYPE, m_anariRendererSubtype);

    settings.beginWriteArray(KANARIPARAMS, m_rendererParameters.size());
    for (int i = 0; i < m_rendererParameters.size(); ++i) {
        settings.setArrayIndex(i);
        const auto& p = m_rendererParameters[i];
        settings.setValue("name", p.name);
        settings.setValue("type", p.type);
        settings.setValue("value", p.value);
    }
    settings.endArray();
    settings.endGroup();
}

void AppMainWindow::createCentralWidget()
{
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_frameWidget = new AnariFrameWidget(m_centralWidget);
    m_mainLayout->addWidget(m_frameWidget, 1);

    m_logWidget = new CollapsibleLogWidget(tr("Log"), kDefaultLogHeight, m_centralWidget);
    m_mainLayout->addWidget(m_logWidget, 0);

    setCentralWidget(m_centralWidget);
}

void AppMainWindow::createRenderer()
{
    m_renderer = new AnariRenderer(this);

    connect(m_renderer, &AnariRenderer::frameReady,
            m_frameWidget, &AnariFrameWidget::updateFrame);
    connect(m_renderer, &AnariRenderer::statusMessage,
            this, &AppMainWindow::onRendererStatusMessage);
    connect(m_renderer, &AnariRenderer::backendLoaded,
            this, &AppMainWindow::onBackendLoaded);
    connect(m_frameWidget, &AnariFrameWidget::resized,
            m_renderer, &AnariRenderer::resize);
}

void AppMainWindow::startBackend()
{
    m_renderer->resize(m_frameWidget->size());
    m_renderer->loadBackend(m_anariLibrary, m_anariDeviceSubtype);
}

void AppMainWindow::onBackendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype)
{
    if (!ok) {
        appendErrorLogMessage(tr("Failed to load ANARI backend \"%1\".").arg(libraryName));
        return;
    }
    m_anariLibrary = libraryName;
    m_anariDeviceSubtype = deviceSubtype;

    m_renderer->setRendererSubtype(m_anariRendererSubtype);
    applyParameters(m_rendererParameters);

    const QString modelPath = QDir(QCoreApplication::applicationDirPath())
                                  .filePath(QStringLiteral("models/viking_room.obj"));
    m_renderer->setSceneFromObj(modelPath);
    m_renderer->start();
}

void AppMainWindow::applyParameters(const QVector<AnariBackendDialog::ParamValue>& parameters)
{
    for (const auto& p : parameters) {
        m_renderer->setRendererParameter(p.name, p.type, p.value);
    }
}

void AppMainWindow::onRendererStatusMessage(int level, const QString& message)
{
    appendLogMessage(message, static_cast<LogLevel>(level));
}

void AppMainWindow::appendLogMessage(const QString& message, LogLevel level)
{
    if (!m_logWidget || message.isEmpty()) {
        return;
    }
    m_logWidget->appendLogMessage(message, level);
}

void AppMainWindow::createActions()
{
    m_closeAction = new QAction(QIcon(QStringLiteral(":/images/power.png")), tr("&Exit"), this);
    connect(m_closeAction, &QAction::triggered, qApp, &QApplication::quit);

    m_aboutAction = new QAction(tr("&About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &AppMainWindow::showAboutDialog);

    m_aboutQtAction = new QAction(tr("About &Qt"), this);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);

    m_preferencesAction = new QAction(tr("&Preferences..."), this);
    connect(m_preferencesAction, &QAction::triggered, this, &AppMainWindow::showPreferencesDialog);

    m_renderingOptionsAction = new QAction(tr("&Rendering..."), this);
    connect(m_renderingOptionsAction, &QAction::triggered, this, &AppMainWindow::showRenderingOptionsDialog);
}

void AppMainWindow::createFileMenu()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAction);
}

void AppMainWindow::createHelpMenu()
{
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
    m_helpMenu->addAction(m_aboutQtAction);
}

void AppMainWindow::createEditMenu()
{
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_preferencesAction);
}

void AppMainWindow::createOptionsMenu()
{
    m_optionsMenu = menuBar()->addMenu(tr("&Options"));
    m_optionsMenu->addAction(m_renderingOptionsAction);
}

void AppMainWindow::showAboutDialog()
{
    QMessageBox::about(this,
                       tr("About Vitrine"),
                       tr("<h2>Vitrine</h2>"
                          "Qt6 application rendering through an ANARI device.<br>"
                          "<p>Copyright &copy; 2026 Dr. Kevin S. Griffin kevin.s.griffin@gmail.com"));
}

void AppMainWindow::showRenderingOptionsDialog()
{
    AnariBackendDialog dialog(m_anariLibrary,
                              m_anariDeviceSubtype,
                              m_anariRendererSubtype,
                              m_rendererParameters,
                              this);
    connect(&dialog, &AnariBackendDialog::configurationChanged,
            this, &AppMainWindow::onBackendDialogConfigurationChanged);
    dialog.exec();
}

void AppMainWindow::onBackendDialogConfigurationChanged(const QString& library,
                                                        const QString& deviceSubtype,
                                                        const QString& rendererSubtype,
                                                        const QVector<AnariBackendDialog::ParamValue>& parameters)
{
    const bool libraryChanged = library != m_anariLibrary || deviceSubtype != m_anariDeviceSubtype;
    const bool rendererChanged = rendererSubtype != m_anariRendererSubtype;

    m_anariLibrary = library;
    m_anariDeviceSubtype = deviceSubtype;
    m_anariRendererSubtype = rendererSubtype;
    m_rendererParameters = parameters;

    if (libraryChanged) {
        // Reload the backend; onBackendLoaded re-applies renderer subtype +
        // parameters + scene once it's ready.
        m_renderer->stop();
        m_renderer->loadBackend(m_anariLibrary, m_anariDeviceSubtype);
    } else {
        if (rendererChanged) {
            m_renderer->setRendererSubtype(m_anariRendererSubtype);
        }
        applyParameters(m_rendererParameters);
    }
    saveSettings();
}

void AppMainWindow::showPreferencesDialog()
{
    if (!m_preferencesDialog) {
        m_preferencesDialog = new QDialog(this);
        m_preferencesDialog->setWindowTitle(tr("Preferences"));
        m_preferencesDialog->setModal(true);

        auto* mainLayout = new QVBoxLayout(m_preferencesDialog);
        mainLayout->addWidget(new QLabel(
            tr("Preferences are configured via the Options menu for now."),
            m_preferencesDialog));

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close,
                                             Qt::Horizontal, m_preferencesDialog);
        connect(buttons, &QDialogButtonBox::rejected, m_preferencesDialog, &QDialog::reject);
        mainLayout->addWidget(buttons);
    }
    m_preferencesDialog->show();
}

} // namespace vitrine
