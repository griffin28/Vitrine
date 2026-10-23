#include "AppMainWindow.h"

#include "AnariFrameWidget.h"
#include "AnariRenderer.h"
#include "overlays/SceneAxisOverlay.h"
#include "dialogs/CameraConfigDialog.h"
#include "dialogs/LightConfigDialog.h"
#include "overlays/LightGizmoOverlay.h"
#include "dialogs/PreferencesDialog.h"
#include "DataLoaderFactory.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStyle>
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
    if (!anariLibrary.isEmpty()) 
    {
        m_anariLibrary = anariLibrary;
    }

    createCentralWidget();
    createRenderer();

    createActions();
    createFileMenu();
    createEditMenu();
    createOptionsMenu();
    createHelpMenu();

    // Apply persisted preferences now that the widgets they target exist.
    applyPreferences();
    startBackend();
}

AppMainWindow::~AppMainWindow()
{
    if (m_renderer) 
    {
        m_renderer->stop();
    }
}

void AppMainWindow::closeEvent(QCloseEvent* event)
{
    // Halt the render loop before anything tears down so we aren't kicking or
    // polling frames mid-shutdown. stop() pauses the timer; destroyBackend()
    // then drains any in-flight frame and releases the ANARI device cleanly.
    if (m_renderer) 
    {
        m_renderer->stop();
        m_renderer->destroyBackend();
    }
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

    settings.beginGroup(KCAMERA);
    m_cameraConfig.type = static_cast<CameraType>(
        settings.value("type", static_cast<int>(m_cameraConfig.type)).toInt());
    m_cameraConfig.eye = {settings.value("eyeX", m_cameraConfig.eye.x).toFloat(),
                          settings.value("eyeY", m_cameraConfig.eye.y).toFloat(),
                          settings.value("eyeZ", m_cameraConfig.eye.z).toFloat()};
    m_cameraConfig.center = {settings.value("centerX", m_cameraConfig.center.x).toFloat(),
                             settings.value("centerY", m_cameraConfig.center.y).toFloat(),
                             settings.value("centerZ", m_cameraConfig.center.z).toFloat()};
    m_cameraConfig.up = {settings.value("upX", m_cameraConfig.up.x).toFloat(),
                         settings.value("upY", m_cameraConfig.up.y).toFloat(),
                         settings.value("upZ", m_cameraConfig.up.z).toFloat()};
    m_cameraConfig.nearClip = settings.value("near", m_cameraConfig.nearClip).toFloat();
    m_cameraConfig.farClip = settings.value("far", m_cameraConfig.farClip).toFloat();
    m_cameraConfig.fovYDegrees = settings.value("fovYDegrees", m_cameraConfig.fovYDegrees).toFloat();
    m_cameraConfig.height = settings.value("height", m_cameraConfig.height).toFloat();
    settings.endGroup();

    // An explicitly saved empty list means "no lights"; only a missing array
    // falls back to the default light.
    if (settings.contains(QStringLiteral("%1/size").arg(KLIGHTS))) 
    {
        m_lightConfigs.clear();
        const int lightCount = settings.beginReadArray(KLIGHTS);
        for (int i = 0; i < lightCount; ++i) 
        {
            settings.setArrayIndex(i);
            LightConfig l;
            l.type = static_cast<LightType>(settings.value("type", 0).toInt());
            l.enabled = settings.value("enabled", true).toBool();
            l.color = {settings.value("colorR", 1.0).toFloat(),
                       settings.value("colorG", 1.0).toFloat(),
                       settings.value("colorB", 1.0).toFloat()};
            l.position = {settings.value("posX", l.position.x).toFloat(),
                          settings.value("posY", l.position.y).toFloat(),
                          settings.value("posZ", l.position.z).toFloat()};
            l.direction = {settings.value("dirX", l.direction.x).toFloat(),
                           settings.value("dirY", l.direction.y).toFloat(),
                           settings.value("dirZ", l.direction.z).toFloat()};
            l.intensity = settings.value("intensity", l.intensity).toFloat();
            l.openingAngleDegrees = settings.value("opening", l.openingAngleDegrees).toFloat();
            l.falloffAngleDegrees = settings.value("falloff", l.falloffAngleDegrees).toFloat();
            m_lightConfigs.push_back(l);
        }
        settings.endArray();
    }

    settings.beginGroup(KPREFS);
    m_preferences.showAxisOverlay =
        settings.value("showAxisOverlay", m_preferences.showAxisOverlay).toBool();
    m_preferences.showLightGizmos =
        settings.value("showLightGizmos", m_preferences.showLightGizmos).toBool();
    settings.endGroup();

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

    settings.beginGroup(KCAMERA);
    settings.setValue("type", static_cast<int>(m_cameraConfig.type));
    settings.setValue("eyeX", m_cameraConfig.eye.x);
    settings.setValue("eyeY", m_cameraConfig.eye.y);
    settings.setValue("eyeZ", m_cameraConfig.eye.z);
    settings.setValue("centerX", m_cameraConfig.center.x);
    settings.setValue("centerY", m_cameraConfig.center.y);
    settings.setValue("centerZ", m_cameraConfig.center.z);
    settings.setValue("upX", m_cameraConfig.up.x);
    settings.setValue("upY", m_cameraConfig.up.y);
    settings.setValue("upZ", m_cameraConfig.up.z);
    settings.setValue("near", m_cameraConfig.nearClip);
    settings.setValue("far", m_cameraConfig.farClip);
    settings.setValue("fovYDegrees", m_cameraConfig.fovYDegrees);
    settings.setValue("height", m_cameraConfig.height);
    settings.endGroup();

    settings.beginWriteArray(KLIGHTS, static_cast<int>(m_lightConfigs.size()));
    for (int i = 0; i < static_cast<int>(m_lightConfigs.size()); ++i) 
    {
        settings.setArrayIndex(i);
        const LightConfig& l = m_lightConfigs[i];
        settings.setValue("type", static_cast<int>(l.type));
        settings.setValue("enabled", l.enabled);
        settings.setValue("colorR", l.color.x);
        settings.setValue("colorG", l.color.y);
        settings.setValue("colorB", l.color.z);
        settings.setValue("posX", l.position.x);
        settings.setValue("posY", l.position.y);
        settings.setValue("posZ", l.position.z);
        settings.setValue("dirX", l.direction.x);
        settings.setValue("dirY", l.direction.y);
        settings.setValue("dirZ", l.direction.z);
        settings.setValue("intensity", l.intensity);
        settings.setValue("opening", l.openingAngleDegrees);
        settings.setValue("falloff", l.falloffAngleDegrees);
    }
    settings.endArray();

    settings.beginGroup(KPREFS);
    settings.setValue("showAxisOverlay", m_preferences.showAxisOverlay);
    settings.setValue("showLightGizmos", m_preferences.showLightGizmos);
    settings.endGroup();

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

    // Axis gizmo floats over the rendered frame (bottom-left corner). It is a
    // child of the frame widget rather than a layout item so it can overlap
    // the image; we reposition it whenever the frame widget resizes.
    m_axisOverlay = new SceneAxisOverlay(m_frameWidget);
    m_axisOverlay->show();

    // Light gizmos cover the whole frame; resized/raised with the frame widget.
    m_lightOverlay = new LightGizmoOverlay(m_frameWidget);
    m_lightOverlay->setLights(m_lightConfigs);
    m_lightOverlay->show();

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

    // Camera control: frame-widget mouse gestures drive the renderer's camera,
    // and the resulting basis feeds the axis overlay.
    connect(m_frameWidget, &AnariFrameWidget::orbitRequested,
            m_renderer, &AnariRenderer::orbitCamera);
    connect(m_frameWidget, &AnariFrameWidget::panRequested,
            m_renderer, &AnariRenderer::panCamera);
    connect(m_frameWidget, &AnariFrameWidget::dollyRequested,
            m_renderer, &AnariRenderer::dollyCamera);
    connect(m_renderer, &AnariRenderer::cameraChanged,
            m_axisOverlay, &SceneAxisOverlay::setBasis);
    connect(m_renderer, &AnariRenderer::cameraMatricesChanged,
            m_lightOverlay, &LightGizmoOverlay::setCamera);

    // Keep the gizmo pinned to the frame widget's bottom-left corner.
    connect(m_frameWidget, &AnariFrameWidget::resized,
            this, [this](const QSize& size) {
                if (m_lightOverlay) 
                {
                    m_lightOverlay->setGeometry(QRect(QPoint(0, 0), size));
                    m_lightOverlay->raise();
                }
                if (m_axisOverlay) 
                {
                    constexpr int margin = 8;
                    m_axisOverlay->move(margin,
                                        size.height() - m_axisOverlay->height() - margin);
                    m_axisOverlay->raise();
                }
            });
}

void AppMainWindow::startBackend()
{
    m_renderer->resize(m_frameWidget->size());
    m_renderer->loadBackend(m_anariLibrary, m_anariDeviceSubtype);
}

void AppMainWindow::onBackendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype)
{
    if (!ok) 
    {
        appendErrorLogMessage(tr("Failed to load ANARI backend \"%1\".").arg(libraryName));
        return;
    }
    m_anariLibrary = libraryName;
    m_anariDeviceSubtype = deviceSubtype;

    m_renderer->setRendererSubtype(m_anariRendererSubtype);
    applyParameters(m_rendererParameters);

    // Push the persisted lights before the first frame is rendered.
    m_renderer->setLightConfigs(m_lightConfigs);

    // Apply the persisted camera now that the ANARI camera handle exists.
    m_renderer->setCameraConfig(m_cameraConfig);

    m_renderer->start();
}

void AppMainWindow::applyParameters(const QVector<AnariBackendDialog::ParamValue>& parameters)
{
    for (const auto& p : parameters) 
    {
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

QIcon AppMainWindow::menuIcon(const QString& resourcePath,
                              const QString& themeName,
                              QStyle::StandardPixmap fallback) const
{
    // Prefer a bundled resource icon, then a platform theme icon, then fall
    // back to a Qt-provided standard pixmap so every item always has an icon.
    if (!resourcePath.isEmpty()) {
        QIcon icon(resourcePath);
        if (!icon.isNull()) {
            return icon;
        }
    }
    if (!themeName.isEmpty() && QIcon::hasThemeIcon(themeName)) {
        return QIcon::fromTheme(themeName);
    }
    return style()->standardIcon(fallback);
}

void AppMainWindow::createActions()
{
    m_openFileAction = new QAction(
        menuIcon(QString(), QStringLiteral("document-open"), QStyle::SP_DirOpenIcon),
        tr("&Open File..."), this);
    m_openFileAction->setShortcut(QKeySequence::Open);
    connect(m_openFileAction, &QAction::triggered, this, &AppMainWindow::openFile);

    m_closeAction = new QAction(
        menuIcon(QStringLiteral(":/images/power.png"), QStringLiteral("application-exit"),
                 QStyle::SP_DialogCloseButton),
        tr("&Exit"), this);
    // Route through close() so closeEvent runs (stops the render loop + saves
    // settings) rather than quitting the event loop out from under it.
    connect(m_closeAction, &QAction::triggered, this, &AppMainWindow::close);

    m_aboutAction = new QAction(
        menuIcon(QString(), QStringLiteral("help-about"), QStyle::SP_DialogHelpButton),
        tr("&About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &AppMainWindow::showAboutDialog);

    m_aboutQtAction = new QAction(
        menuIcon(QString(), QString(), QStyle::SP_TitleBarMenuButton),
        tr("About &Qt"), this);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);

    m_preferencesAction = new QAction(
        menuIcon(QString(), QStringLiteral("preferences-system"),
                 QStyle::SP_FileDialogDetailedView),
        tr("&Preferences..."), this);
    connect(m_preferencesAction, &QAction::triggered, this, &AppMainWindow::showPreferencesDialog);

    m_renderingOptionsAction = new QAction(
        menuIcon(QString(), QStringLiteral("applications-graphics"),
                 QStyle::SP_DesktopIcon),
        tr("&Rendering..."), this);
    connect(m_renderingOptionsAction, &QAction::triggered, this, &AppMainWindow::showRenderingOptionsDialog);

    m_cameraOptionsAction = new QAction(
        menuIcon(QString(), QStringLiteral("camera-photo"), QStyle::SP_FileDialogContentsView),
        tr("&Camera..."), this);
    connect(m_cameraOptionsAction, &QAction::triggered, this, &AppMainWindow::showCameraOptionsDialog);

    m_lightOptionsAction = new QAction(
        menuIcon(QString(), QStringLiteral("weather-clear"), QStyle::SP_MessageBoxInformation),
        tr("&Lights..."), this);
    connect(m_lightOptionsAction, &QAction::triggered, this, &AppMainWindow::showLightOptionsDialog);
}

void AppMainWindow::createFileMenu()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openFileAction);
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
    m_optionsMenu = menuBar()->addMenu(tr("&Anari"));
    m_optionsMenu->addAction(m_renderingOptionsAction);
    m_optionsMenu->addAction(m_cameraOptionsAction);
    m_optionsMenu->addAction(m_lightOptionsAction);
}

void AppMainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(), DataLoaderFactory::fileFilters().join(QStringLiteral(";;")));
    if (path.isEmpty()) {
        return;
    }

    // DataLoaderFactory selects a loader by suffix and logs unsupported types.
    m_renderer->loadSceneFromFile(path);
}

void AppMainWindow::showAboutDialog()
{
    QMessageBox::about(this,
                       tr("About Vitrine"),
                       tr("<h2>Vitrine</h2>"
                          "Qt6 application rendering through an ANARI device.<br>"
                          "<p>Copyright &copy; 2036 Dr. Kevin S. Griffin kevin.s.griffin@gmail.com"));
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

    if (libraryChanged) 
    {
        // Reload the backend; onBackendLoaded re-applies renderer subtype +
        // parameters + scene once it's ready.
        m_renderer->stop();
        m_renderer->loadBackend(m_anariLibrary, m_anariDeviceSubtype);
    } 
    else 
    {
        if (rendererChanged) 
        {
            m_renderer->setRendererSubtype(m_anariRendererSubtype);
        }
        applyParameters(m_rendererParameters);
    }
    saveSettings();
}

void AppMainWindow::showCameraOptionsDialog()
{
    // Seed from the renderer's live camera so the dialog reflects any orbiting
    // / panning the user has done since launch.
    CameraConfigDialog dialog(m_renderer->cameraConfig(), this);
    connect(&dialog, &CameraConfigDialog::cameraConfigChanged,
            this, &AppMainWindow::onCameraConfigChanged);
    dialog.exec();
}

void AppMainWindow::onCameraConfigChanged(const CameraConfig& config)
{
    m_cameraConfig = config;
    m_renderer->setCameraConfig(config);
    saveSettings();
}

void AppMainWindow::showLightOptionsDialog()
{
    LightConfigDialog dialog(m_lightConfigs, m_renderer->supportedLightTypes(), this);
    connect(&dialog, &LightConfigDialog::lightConfigsChanged,
            this, &AppMainWindow::onLightConfigsChanged);
    dialog.exec();
}

void AppMainWindow::onLightConfigsChanged(const std::vector<LightConfig>& lights)
{
    m_lightConfigs = lights;
    m_renderer->setLightConfigs(lights);
    m_lightOverlay->setLights(lights);
    saveSettings();
}

void AppMainWindow::showPreferencesDialog()
{
    PreferencesDialog dialog(m_preferences, this);
    connect(&dialog, &PreferencesDialog::preferencesChanged,
            this, &AppMainWindow::onPreferencesChanged);
    dialog.exec();
}

void AppMainWindow::onPreferencesChanged(const UserPreferences& preferences)
{
    m_preferences = preferences;
    applyPreferences();
    saveSettings();
}

void AppMainWindow::applyPreferences()
{
    if (m_axisOverlay) {
        m_axisOverlay->setVisible(m_preferences.showAxisOverlay);
    }
    if (m_lightOverlay) {
        m_lightOverlay->setVisible(m_preferences.showLightGizmos);
    }
}

} // namespace vitrine
