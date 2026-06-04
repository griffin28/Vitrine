#include "AnariBackendDialog.h"

#include <anari/anari.h>

#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <QVBoxLayout>

#include <array>
#include <utility>

namespace vitrine
{

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
    for (const QString& lib : AnariUtils::enumerateBackendLibraries()) 
    {
        m_libraryCombo->addItem(lib);
    }

    m_selectedLibrary = m_selectedLibrary.isEmpty() ? QStringLiteral("phenocryst") : m_selectedLibrary;
    int initialLibIndex = m_libraryCombo->findText(m_selectedLibrary);
    if (initialLibIndex < 0) 
    {
        m_libraryCombo->insertItem(0, m_selectedLibrary);
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

void AnariBackendDialog::onLibraryChanged(int index)
{
    releaseProbe();
    m_deviceSubtypeCombo->clear();
    m_rendererSubtypeCombo->clear();

    const QString library = m_libraryCombo->itemText(index);
    if (library.isEmpty()) 
    {
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
    if (!m_probeLibrary) 
    {
        m_parameterForm->addRow(new QLabel(
            tr("Could not load ANARI library \"%1\". Check ANARI_LIBRARY_PATH.")
                .arg(library)));
        return;
    }

    for (const QString& sub : AnariUtils::enumerateDeviceSubtypes(m_probeLibrary)) 
    {
        m_deviceSubtypeCombo->addItem(sub);
    }
    if (m_deviceSubtypeCombo->count() == 0) 
    {
        m_deviceSubtypeCombo->addItem(QStringLiteral("default"));
    }
    int devIdx = m_deviceSubtypeCombo->findText(m_selectedDeviceSubtype);
    if (devIdx < 0) 
    {
        devIdx = 0;
    }
    m_deviceSubtypeCombo->setCurrentIndex(devIdx);

    onDeviceSubtypeChanged(devIdx);
}

void AnariBackendDialog::onDeviceSubtypeChanged(int index)
{
    if (m_probeDevice) 
    {
        anariRelease(m_probeDevice, m_probeDevice);
        m_probeDevice = nullptr;
    }
    m_rendererSubtypeCombo->clear();

    if (!m_probeLibrary) 
    {
        return;
    }
    const QString subtype = m_deviceSubtypeCombo->itemText(index);
    if (subtype.isEmpty()) 
    {
        return;
    }
    const QByteArray utf8 = subtype.toUtf8();
    m_probeDevice = anariNewDevice(m_probeLibrary, utf8.constData());
    if (!m_probeDevice) {
        return;
    }
    anariCommitParameters(m_probeDevice, m_probeDevice);

    for (const QString& sub : AnariUtils::enumerateRendererSubtypes(m_probeDevice)) 
    {
        m_rendererSubtypeCombo->addItem(sub);
    }
    if (m_rendererSubtypeCombo->count() == 0) 
    {
        m_rendererSubtypeCombo->addItem(QStringLiteral("default"));
    }
    int rendIdx = m_rendererSubtypeCombo->findText(m_selectedRendererSubtype);
    if (rendIdx < 0) 
    {
        rendIdx = 0;
    }
    m_rendererSubtypeCombo->setCurrentIndex(rendIdx);
    onRendererSubtypeChanged(rendIdx);
}

void AnariBackendDialog::onRendererSubtypeChanged(int index)
{
    buildParameterPanel(index);
}

void AnariBackendDialog::buildParameterPanel(const int index)
{
    // Clear current form.
    while (m_parameterForm->rowCount() > 0) 
    {
        m_parameterForm->removeRow(0);
    }
    m_editors.clear();

    if (!m_probeDevice) 
    {
        return;
    }
    const QString rendererSubtype = m_rendererSubtypeCombo->itemText(index);
    if (rendererSubtype.isEmpty()) 
    {
        return;
    }

    const auto params = AnariUtils::enumerateRendererParameters(m_probeDevice, rendererSubtype);
    if (params.empty()) 
    {
        m_parameterForm->addRow(new QLabel(
            tr("(Renderer \"%1\" exposes no parameters via introspection.)")
                .arg(rendererSubtype)));
        return;
    }

    for (const auto& p : params) 
    {
        // Seed from the caller-provided initial values if the names match.
        QVariant initial;
        QString pname = p.getName();
        ANARIDataType ptype = p.getType();
        QString pdescription = p.getDescription();

        for (const auto& iv : m_initialParameters) 
        {
            if (iv.name == pname && iv.type == static_cast<int>(ptype)) 
            {
                initial = iv.value;
                break;
            }
        }

        QWidget* widget = makeWidgetForParameter(p, initial);
        if (!widget) 
        {
            continue;
        }

        QLabel *label = new QLabel(pname, this);
        
        if (!pdescription.isEmpty()) 
        {
            label->setToolTip(pdescription);
        }
        m_parameterForm->addRow(label, widget);
        m_editors.push_back({pname, static_cast<int>(ptype), widget});
    }
}

QWidget* AnariBackendDialog::makeWidgetForParameter(const AnariRendererParameter& p, const QVariant& current)
{
    const auto type = p.getType();
    const auto name = p.getName();

    // Ignore ANARI metadata and handle-like types that cannot be edited here.
    if (type < ANARI_INT8)
    {
        return nullptr;
    }

    if ( (name.contains("background", Qt::CaseInsensitive) ||
          name.contains("ambientRadiance", Qt::CaseInsensitive) ||
          name.contains("ambientColor", Qt::CaseInsensitive) ||
          name.contains("color", Qt::CaseInsensitive)) )
    {
        auto* container = new QWidget(this);
        auto* hbox = new QHBoxLayout(container);
        hbox->setContentsMargins(0, 0, 0, 0);
        auto* button = this->createColorButton(container, type, current);
        hbox->addWidget(button);

        return container;
    }

    switch (type) {
        case ANARI_FLOAT32: 
            return this->getSpinBoxContainer<float>(p, current, 1, this);
        case ANARI_FLOAT64:
            return this->getSpinBoxContainer<double>(p, current, 1, this);
        case ANARI_INT32: 
            return this->getSpinBoxContainer<int>(p, current, 1, this);
        case ANARI_BOOL: 
        {
            auto* checkBox = new QCheckBox(this);

            if (current.isValid())
            {
                checkBox->setChecked(current.toBool());
            }
            else if (p.hasDefaultValue())
            {
                auto boolPtr = p.getDefaultValue<const int32_t *>();
                checkBox->setChecked(boolPtr && *boolPtr);
            }
            
            return checkBox;
        }
        case ANARI_FLOAT32_VEC2:
            return this->getSpinBoxContainer<float>(p, current, 2, this);
        case ANARI_FLOAT32_VEC3:
            return this->getSpinBoxContainer<float>(p, current, 3, this);
        case ANARI_FLOAT32_VEC4: 
            return this->getSpinBoxContainer<float>(p, current, 4, this);
        case ANARI_FLOAT64_VEC2:
            return this->getSpinBoxContainer<double>(p, current, 2, this);
        case ANARI_FLOAT64_VEC3:
            return this->getSpinBoxContainer<double>(p, current, 3, this);
        case ANARI_FLOAT64_VEC4: 
            return this->getSpinBoxContainer<double>(p, current, 4, this);
        case ANARI_INT32_VEC2:
            return this->getSpinBoxContainer<int>(p, current, 2, this);
        case ANARI_INT32_VEC3:
            return this->getSpinBoxContainer<int>(p, current, 3, this);
        case ANARI_INT32_VEC4: 
            return this->getSpinBoxContainer<int>(p, current, 4, this);
        case ANARI_STRING: 
        {
            auto acceptedVals = p.getAcceptedValues();

            if (!acceptedVals.isEmpty()) 
            {
                auto* comboBox = new QComboBox(this);
                for (const QString &val : acceptedVals)
                {
                    comboBox->addItem(val);
                }

                if (current.isValid() && current.canConvert<QString>())
                {
                    const int index = comboBox->findText(current.toString());
                    if (index >=0 )
                    {
                        comboBox->setCurrentIndex(index);
                    }
                }

                return comboBox;
            }
            else 
            {
                auto* edit = new QLineEdit(this);

                if (current.isValid() && current.canConvert<QString>())
                {
                    edit->setText(current.toString());
                }
                return edit;
            }
        }
        default: 
        {
            auto* label = new QLabel(tr("(unsupported type %1 for \"%2\")")
                                         .arg(type).arg(name), this);
            label->setEnabled(false);
            return label;
            // auto* edit = new QLineEdit(this);

            // if (current.isValid() && current.canConvert<QString>())
            // {
            //     edit->setText(current.toString());
            // }
            
            // return edit;
        }
    }
}

QVariant AnariBackendDialog::readWidget(int type, QWidget* editor) const
{
    if (!editor) {
        return {};
    }

    if (auto* button = editor->findChild<QPushButton*>(QString(), Qt::FindDirectChildrenOnly)) 
    {
        const QVariant color = button->property("color");
        if (color.isValid()) {
            return color;
        }
    }

    const auto readDoubleSpinBoxes = [editor](int components) -> QVariant 
    {
        if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
            return components == 1 ? QVariant(spin->value()) : QVariant();
        }

        const auto spins = editor->findChildren<QDoubleSpinBox*>(
            QString(), Qt::FindDirectChildrenOnly);
        if (spins.size() != components) {
            return {};
        }
        if (components == 1) {
            return spins[0]->value();
        }

        QVariantList values;
        values.reserve(components);
        for (auto* spin : spins) {
            values.push_back(spin->value());
        }

        return values;
    };

    const auto readIntSpinBoxes = [editor](int components) -> QVariant 
    {
        if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
            return components == 1 ? QVariant(spin->value()) : QVariant();
        }

        const auto spins = editor->findChildren<QSpinBox*>(
            QString(), Qt::FindDirectChildrenOnly);
        if (spins.size() != components) {
            return {};
        }
        if (components == 1) {
            return spins[0]->value();
        }

        QVariantList values;
        values.reserve(components);
        for (auto* spin : spins) {
            values.push_back(spin->value());
        }

        return values;
    };

    switch (type) {
        case ANARI_FLOAT32:
        case ANARI_FLOAT64:
            return readDoubleSpinBoxes(1);
        case ANARI_INT32:
            return readIntSpinBoxes(1);
        case ANARI_BOOL:
            if (auto* c = qobject_cast<QCheckBox*>(editor)) 
            {
                return c->isChecked();
            }
            break;
        case ANARI_FLOAT32_VEC2:
        case ANARI_FLOAT64_VEC2:
            return readDoubleSpinBoxes(2);
        case ANARI_FLOAT32_VEC3:
        case ANARI_FLOAT64_VEC3:
            return readDoubleSpinBoxes(3);
        case ANARI_FLOAT32_VEC4:
        case ANARI_FLOAT64_VEC4:
            return readDoubleSpinBoxes(4);
        case ANARI_INT32_VEC2:
            return readIntSpinBoxes(2);
        case ANARI_INT32_VEC3:
            return readIntSpinBoxes(3);
        case ANARI_INT32_VEC4:
            return readIntSpinBoxes(4);
        case ANARI_STRING:
            if (auto* l = qobject_cast<QLineEdit*>(editor)) {
                return l->text();
            }
            if (auto* c = qobject_cast<QComboBox*>(editor)) {
                return c->currentText();
            }
            break;
        default:
            break;
    }
    return {};
}

QPushButton* AnariBackendDialog::createColorButton(QWidget* parent, ANARIDataType type, const QVariant& current)
{
    auto* button = new QPushButton(parent);
    QColor initial = current.isValid() ? AnariBackendDialog::variantToColor(type, current) : QColor(Qt::black);
    AnariBackendDialog::setColorButtonSwatch(button, initial);
    button->setProperty("color", initial);
    connect(button, &QPushButton::clicked, this, [button, parent]() {
        QColor c = button->property("color").value<QColor>();
        QColor picked = QColorDialog::getColor(c, parent,
                                               tr("Pick parameter color"),
                                               QColorDialog::ShowAlphaChannel);
        if (picked.isValid()) 
        {
            button->setProperty("color", picked);
            AnariBackendDialog::setColorButtonSwatch(button, picked);
        }
    });

    return button;
}

QColor AnariBackendDialog::variantToColor(ANARIDataType type, const QVariant& v)
{
    if (v.canConvert<QColor>()) {
        return v.value<QColor>();
    }
    
    const auto list = v.toList();
    switch(type){
        case ANARI_UFIXED8_VEC3:
        case ANARI_UFIXED8_VEC4:
        {
            QColor c;
            if (list.size() == 3)
            {
                c.setRgb(std::clamp(list[0].toInt(), 0, 255),
                         std::clamp(list[1].toInt(), 0, 255),
                         std::clamp(list[2].toInt(), 0, 255));
                return c;
            }
            else if (list.size() == 4)
            {
                c.setRgb(std::clamp(list[0].toInt(), 0, 255),
                         std::clamp(list[1].toInt(), 0, 255),
                         std::clamp(list[2].toInt(), 0, 255),
                         std::clamp(list[3].toInt(), 0, 255));
                return c;
            }
        }
        case ANARI_FLOAT32_VEC3:
        case ANARI_FLOAT32_VEC4:
        {
            QColor c;
            if (list.size() == 3)
            {
                c.setRgbF(std::clamp(list[0].toDouble(), 0.0, 1.0),
                          std::clamp(list[1].toDouble(), 0.0, 1.0),
                          std::clamp(list[2].toDouble(), 0.0, 1.0));
                return c;
            }
            else if (list.size() == 4) 
            {
                c.setRgbF(std::clamp(list[0].toDouble(), 0.0, 1.0),
                          std::clamp(list[1].toDouble(), 0.0, 1.0),
                          std::clamp(list[2].toDouble(), 0.0, 1.0),
                          std::clamp(list[3].toDouble(), 0.0, 1.0));
                return c;
            }
        }
        default:
            return QColor(Qt::black);
    }

    return QColor(Qt::black);
}

void AnariBackendDialog::setColorButtonSwatch(QPushButton* button, const QColor& color)
{
    button->setText(color.name(QColor::HexArgb));
    QString style = QStringLiteral(
        "QPushButton { background-color: %1; color: %2; }")
        .arg(color.name(),
             color.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white"));
    button->setStyleSheet(style);
}

void AnariBackendDialog::onAccepted()
{
    m_selectedLibrary = m_libraryCombo->currentText();
    m_selectedDeviceSubtype = m_deviceSubtypeCombo->currentText();
    m_selectedRendererSubtype = m_rendererSubtypeCombo->currentText();

    m_selectedParameters.clear();
    for (const auto& entry : m_editors) {
        QVariant v = readWidget(entry.type, entry.editor);
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
