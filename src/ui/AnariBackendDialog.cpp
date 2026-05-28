#include "AnariBackendDialog.h"

#include "AnariUtils.h"

#include <anari/anari.h>

#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <utility>

namespace vitrine
{

namespace
{

QColor variantToColor(const QVariant& v)
{
    if (v.canConvert<QColor>()) {
        return v.value<QColor>();
    }
    const auto list = v.toList();
    if (list.size() == 4) {
        QColor c;
        c.setRgbF(std::clamp(list[0].toDouble(), 0.0, 1.0),
                  std::clamp(list[1].toDouble(), 0.0, 1.0),
                  std::clamp(list[2].toDouble(), 0.0, 1.0),
                  std::clamp(list[3].toDouble(), 0.0, 1.0));
        return c;
    }
    return QColor(Qt::black);
}

void setColorButtonSwatch(QPushButton* button, const QColor& color)
{
    button->setText(color.name(QColor::HexArgb));
    QString style = QStringLiteral(
        "QPushButton { background-color: %1; color: %2; }")
        .arg(color.name(),
             color.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white"));
    button->setStyleSheet(style);
}

} // namespace

AnariBackendDialog::AnariBackendDialog(const QString& initialLibrary,
                                       const QString& initialDeviceSubtype,
                                       const QString& initialRendererSubtype,
                                       const QVector<ParamValue>& initialParameters,
                                       QWidget* parent)
    : QDialog(parent),
      m_selectedLibrary(initialLibrary),
      m_selectedDeviceSubtype(initialDeviceSubtype),
      m_selectedRendererSubtype(initialRendererSubtype),
      m_selectedParameters(initialParameters),
      m_initialParameters(initialParameters)
{
    setWindowTitle(tr("Rendering — ANARI Backend"));
    setModal(true);
    setMinimumWidth(480);

    auto* mainLayout = new QVBoxLayout(this);
    auto* topForm = new QFormLayout();

    m_libraryCombo = new QComboBox(this);
    for (const QString& lib : AnariUtils::enumerateBackendLibraries()) {
        m_libraryCombo->addItem(lib);
    }
    if (m_libraryCombo->findText(initialLibrary) < 0 && !initialLibrary.isEmpty()) {
        m_libraryCombo->insertItem(0, initialLibrary);
    }
    int initialLibIndex = m_libraryCombo->findText(
        initialLibrary.isEmpty() ? QStringLiteral("phenocryst") : initialLibrary);
    if (initialLibIndex < 0) {
        m_libraryCombo->insertItem(0, initialLibrary);
        initialLibIndex = 0;
    }
    m_libraryCombo->setCurrentIndex(initialLibIndex);
    topForm->addRow(new QLabel(tr("Backend library:")), m_libraryCombo);

    m_deviceSubtypeCombo = new QComboBox(this);
    topForm->addRow(new QLabel(tr("Device subtype:")), m_deviceSubtypeCombo);

    m_rendererSubtypeCombo = new QComboBox(this);
    topForm->addRow(new QLabel(tr("Renderer subtype:")), m_rendererSubtypeCombo);

    mainLayout->addLayout(topForm);

    m_parameterContainer = new QWidget(this);
    m_parameterForm = new QFormLayout(m_parameterContainer);
    m_parameterForm->setContentsMargins(0, 6, 0, 6);
    mainLayout->addWidget(m_parameterContainer);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    connect(m_libraryCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AnariBackendDialog::onLibraryChanged);
    connect(m_deviceSubtypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AnariBackendDialog::onDeviceSubtypeChanged);
    connect(m_rendererSubtypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AnariBackendDialog::onRendererSubtypeChanged);
    connect(this, &QDialog::accepted, this, &AnariBackendDialog::onAccepted);

    // Trigger the cascade for the initial selection.
    onLibraryChanged(m_libraryCombo->currentIndex());
}

AnariBackendDialog::~AnariBackendDialog()
{
    releaseProbe();
}

void AnariBackendDialog::releaseProbe()
{
    if (m_probeDevice) {
        anariRelease(m_probeDevice, m_probeDevice);
        m_probeDevice = nullptr;
    }
    if (m_probeLibrary) {
        anariUnloadLibrary(m_probeLibrary);
        m_probeLibrary = nullptr;
    }
    m_probeStatusSink.reset();
}

void AnariBackendDialog::onLibraryChanged(int /*comboIndex*/)
{
    releaseProbe();
    m_deviceSubtypeCombo->clear();
    m_rendererSubtypeCombo->clear();

    const QString library = m_libraryCombo->currentText();
    if (library.isEmpty()) {
        return;
    }

    // Probe sink: swallow messages so we don't spam the main log during
    // dialog probing. We could surface these as a status bar; minimal for now.
    m_probeStatusSink = std::make_unique<AnariStatusSink>(
        [](LogLevel, QString) {});

    const QByteArray libUtf8 = library.toUtf8();
    m_probeLibrary = anariLoadLibrary(libUtf8.constData(),
                                      m_probeStatusSink->callback(),
                                      m_probeStatusSink->userData());
    if (!m_probeLibrary) {
        m_parameterForm->addRow(new QLabel(
            tr("Could not load ANARI library \"%1\". Check ANARI_LIBRARY_PATH.")
                .arg(library)));
        return;
    }

    for (const QString& sub : AnariUtils::enumerateDeviceSubtypes(m_probeLibrary)) {
        m_deviceSubtypeCombo->addItem(sub);
    }
    if (m_deviceSubtypeCombo->count() == 0) {
        m_deviceSubtypeCombo->addItem(QStringLiteral("default"));
    }
    int devIdx = m_deviceSubtypeCombo->findText(m_selectedDeviceSubtype);
    if (devIdx < 0) {
        devIdx = 0;
    }
    m_deviceSubtypeCombo->setCurrentIndex(devIdx);

    onDeviceSubtypeChanged(devIdx);
}

void AnariBackendDialog::onDeviceSubtypeChanged(int /*comboIndex*/)
{
    if (m_probeDevice) {
        anariRelease(m_probeDevice, m_probeDevice);
        m_probeDevice = nullptr;
    }
    m_rendererSubtypeCombo->clear();

    if (!m_probeLibrary) {
        return;
    }
    const QString subtype = m_deviceSubtypeCombo->currentText();
    if (subtype.isEmpty()) {
        return;
    }
    const QByteArray utf8 = subtype.toUtf8();
    m_probeDevice = anariNewDevice(m_probeLibrary, utf8.constData());
    if (!m_probeDevice) {
        return;
    }
    anariCommitParameters(m_probeDevice, m_probeDevice);

    for (const QString& sub : AnariUtils::enumerateRendererSubtypes(m_probeDevice)) {
        m_rendererSubtypeCombo->addItem(sub);
    }
    if (m_rendererSubtypeCombo->count() == 0) {
        m_rendererSubtypeCombo->addItem(QStringLiteral("default"));
    }
    int rendIdx = m_rendererSubtypeCombo->findText(m_selectedRendererSubtype);
    if (rendIdx < 0) {
        rendIdx = 0;
    }
    m_rendererSubtypeCombo->setCurrentIndex(rendIdx);
    onRendererSubtypeChanged(rendIdx);
}

void AnariBackendDialog::onRendererSubtypeChanged(int /*comboIndex*/)
{
    buildParameterPanel();
}

void AnariBackendDialog::buildParameterPanel()
{
    // Clear current form.
    while (m_parameterForm->rowCount() > 0) {
        m_parameterForm->removeRow(0);
    }
    m_editors.clear();

    if (!m_probeDevice) {
        return;
    }
    const QString rendererSubtype = m_rendererSubtypeCombo->currentText();
    if (rendererSubtype.isEmpty()) {
        return;
    }

    const auto params = AnariUtils::enumerateRendererParameters(m_probeDevice, rendererSubtype);
    if (params.empty()) {
        m_parameterForm->addRow(new QLabel(
            tr("(Renderer \"%1\" exposes no parameters via introspection.)")
                .arg(rendererSubtype)));
        return;
    }

    for (const auto& p : params) {
        // Seed from the caller-provided initial values if the names match.
        QVariant initial;
        for (const auto& iv : m_initialParameters) {
            if (iv.name == p.name && iv.type == static_cast<int>(p.type)) {
                initial = iv.value;
                break;
            }
        }
        QWidget* editor = makeEditorForParameter(static_cast<int>(p.type), p.name, initial);
        if (!editor) {
            continue;
        }
        QString label = p.name;
        if (!p.description.isEmpty()) {
            label = QStringLiteral("%1\n(%2)").arg(p.name, p.description);
        }
        m_parameterForm->addRow(label, editor);
        m_editors.push_back({p.name, static_cast<int>(p.type), editor});
    }
}

QWidget* AnariBackendDialog::makeEditorForParameter(int type, const QString& name, const QVariant& current)
{
    switch (type) {
        case ANARI_FLOAT32: {
            auto* spin = new QDoubleSpinBox(this);
            spin->setDecimals(4);
            spin->setRange(-1e6, 1e6);
            spin->setSingleStep(0.1);
            if (current.isValid()) {
                spin->setValue(current.toDouble());
            }
            return spin;
        }
        case ANARI_INT32: {
            auto* spin = new QSpinBox(this);
            spin->setRange(-1'000'000, 1'000'000);
            if (current.isValid()) {
                spin->setValue(current.toInt());
            }
            return spin;
        }
        case ANARI_UINT32: {
            auto* spin = new QSpinBox(this);
            spin->setRange(0, 1'000'000);
            if (current.isValid()) {
                spin->setValue(current.toInt());
            }
            return spin;
        }
        case ANARI_BOOL: {
            auto* check = new QCheckBox(this);
            check->setChecked(current.toBool());
            return check;
        }
        case ANARI_FLOAT32_VEC4: {
            // Treat any FLOAT32_VEC4 as a colour for now (the only such
            // parameter Phenocryst exposes is "background").
            auto* container = new QWidget(this);
            auto* hbox = new QHBoxLayout(container);
            hbox->setContentsMargins(0, 0, 0, 0);
            auto* button = new QPushButton(container);
            QColor initial = current.isValid() ? variantToColor(current) : QColor(Qt::black);
            setColorButtonSwatch(button, initial);
            button->setProperty("color", initial);
            connect(button, &QPushButton::clicked, this, [button, container]() {
                QColor c = button->property("color").value<QColor>();
                QColor picked = QColorDialog::getColor(c, container,
                                                      tr("Pick parameter color"),
                                                      QColorDialog::ShowAlphaChannel);
                if (picked.isValid()) {
                    button->setProperty("color", picked);
                    setColorButtonSwatch(button, picked);
                }
            });
            hbox->addWidget(button);
            return container;
        }
        case ANARI_STRING: {
            auto* edit = new QLineEdit(this);
            edit->setText(current.toString());
            return edit;
        }
        default: {
            auto* label = new QLabel(tr("(unsupported parameter type %1 for \"%2\")")
                                         .arg(type).arg(name), this);
            label->setEnabled(false);
            return label;
        }
    }
}

QVariant AnariBackendDialog::readEditor(int type, QWidget* editor) const
{
    switch (type) {
        case ANARI_FLOAT32:
            if (auto* s = qobject_cast<QDoubleSpinBox*>(editor)) {
                return s->value();
            }
            break;
        case ANARI_INT32:
        case ANARI_UINT32:
            if (auto* s = qobject_cast<QSpinBox*>(editor)) {
                return s->value();
            }
            break;
        case ANARI_BOOL:
            if (auto* c = qobject_cast<QCheckBox*>(editor)) {
                return c->isChecked();
            }
            break;
        case ANARI_FLOAT32_VEC4: {
            // The editor for VEC4 is a container; the QPushButton inside
            // holds the chosen QColor in a dynamic property.
            if (editor) {
                auto* button = editor->findChild<QPushButton*>();
                if (button) {
                    return button->property("color");
                }
            }
            break;
        }
        case ANARI_STRING:
            if (auto* l = qobject_cast<QLineEdit*>(editor)) {
                return l->text();
            }
            break;
        default:
            break;
    }
    return {};
}

void AnariBackendDialog::onAccepted()
{
    m_selectedLibrary = m_libraryCombo->currentText();
    m_selectedDeviceSubtype = m_deviceSubtypeCombo->currentText();
    m_selectedRendererSubtype = m_rendererSubtypeCombo->currentText();

    m_selectedParameters.clear();
    for (const auto& entry : m_editors) {
        QVariant v = readEditor(entry.type, entry.editor);
        if (v.isValid()) {
            m_selectedParameters.push_back({entry.name, entry.type, v});
        }
    }

    emit configurationChanged(m_selectedLibrary,
                              m_selectedDeviceSubtype,
                              m_selectedRendererSubtype,
                              m_selectedParameters);
}

} // namespace vitrine
