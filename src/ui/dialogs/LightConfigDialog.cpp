#include "LightConfigDialog.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace vitrine
{

namespace
{
QString typeName(LightType type)
{
    switch (type) 
    {
        case LightType::Point: return QObject::tr("Point");
        case LightType::Spot: return QObject::tr("Spot");
        case LightType::Directional:
        default: return QObject::tr("Directional");
    }
}

QColor toQColor(const glm::vec3& c)
{
    const auto ch = [](float v) { return std::clamp(static_cast<int>(v * 255.0f + 0.5f), 0, 255); };
    return QColor(ch(c.x), ch(c.y), ch(c.z));
}
} // namespace

LightConfigDialog::LightConfigDialog(const std::vector<LightConfig>& initial,
                                     const std::vector<LightType>& supportedTypes,
                                     QWidget* parent)
    : QDialog(parent)
    , m_lights(initial)
    , m_supportedTypes(supportedTypes)
{
    setWindowTitle(tr("Light Configuration"));
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    auto* body = new QHBoxLayout();
    mainLayout->addLayout(body);

    // Left: list + add/remove.
    auto* left = new QVBoxLayout();
    body->addLayout(left);
    const QString noSupport =
        tr("The active backend does not support directional, point or spot lights.");
    if (m_supportedTypes.empty()) 
    {
        auto* note = new QLabel(noSupport, this);
        note->setWordWrap(true);
        left->addWidget(note);
    }
    m_list = new QListWidget(this);
    m_list->setMinimumWidth(160);
    left->addWidget(m_list, 1);

    auto* buttonRow = new QHBoxLayout();
    left->addLayout(buttonRow);
    auto* addButton = new QPushButton(tr("Add"), this);
    auto* addMenu = new QMenu(addButton);
    for (LightType t : m_supportedTypes) 
    {
        addMenu->addAction(typeName(t), this, [this, t] { addLight(t); });
    }
    addButton->setMenu(addMenu);
    if (m_supportedTypes.empty()) 
    {
        addButton->setEnabled(false);
        addButton->setToolTip(noSupport);
    }
    m_removeButton = new QPushButton(tr("Remove"), this);
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(m_removeButton);

    // Right: property form.
    m_formWidget = new QWidget(this);
    m_form = new QFormLayout(m_formWidget);
    body->addWidget(m_formWidget, 1);

    m_enabledCheck = new QCheckBox(tr("Enabled"), m_formWidget);
    m_form->addRow(m_enabledCheck);

    m_typeCombo = new QComboBox(m_formWidget);
    populateTypeCombo(m_supportedTypes.empty() ? LightType::Directional : m_supportedTypes.front());
    m_form->addRow(tr("Type: "), m_typeCombo);

    m_colorButton = new QPushButton(m_formWidget);
    m_form->addRow(tr("Color: "), m_colorButton);

    m_position = makeVec3Editor(m_positionRow);
    m_form->addRow(tr("Position: "), m_positionRow);
    m_direction = makeVec3Editor(m_directionRow);
    m_form->addRow(tr("Direction: "), m_directionRow);

    m_intensitySpin = new QDoubleSpinBox(m_formWidget);
    m_intensitySpin->setDecimals(3);
    m_intensitySpin->setRange(0.0, 1.0e6);
    m_intensitySpin->setSingleStep(0.1);
    m_form->addRow(tr("Intensity: "), m_intensitySpin);

    m_openingSpin = new QDoubleSpinBox(m_formWidget);
    m_openingSpin->setRange(1.0, 179.0);
    m_openingSpin->setSuffix(QStringLiteral("°"));
    m_form->addRow(tr("Opening Angle: "), m_openingSpin);

    m_falloffSpin = new QDoubleSpinBox(m_formWidget);
    m_falloffSpin->setRange(0.0, 179.0);
    m_falloffSpin->setSuffix(QStringLiteral("°"));
    m_form->addRow(tr("Falloff Angle: "), m_falloffSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &LightConfigDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &LightConfigDialog::onApply);

    connect(m_removeButton, &QPushButton::clicked, this, &LightConfigDialog::removeSelected);
    connect(m_list, &QListWidget::currentRowChanged, this, &LightConfigDialog::onRowChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &LightConfigDialog::pickColor);
    connect(m_enabledCheck, &QCheckBox::toggled, this, &LightConfigDialog::onFieldEdited);
    connect(m_typeCombo, &QComboBox::currentIndexChanged, this, &LightConfigDialog::onFieldEdited);
    for (auto* spin : {m_intensitySpin, m_openingSpin, m_falloffSpin}) 
    {
        connect(spin, &QDoubleSpinBox::valueChanged, this, &LightConfigDialog::onFieldEdited);
    }
    for (const Vec3Editor* editor : {&m_position, &m_direction}) 
    {
        for (auto* spin : *editor) 
        {
            connect(spin, &QDoubleSpinBox::valueChanged, this, &LightConfigDialog::onFieldEdited);
        }
    }

    for (size_t i = 0; i < m_lights.size(); ++i) 
    {
        m_list->addItem(labelFor(static_cast<int>(i), m_lights[i]));
    }
    if (!m_lights.empty()) 
    {
        m_list->setCurrentRow(0);
    } 
    else 
    {
        updateEnabledState();
    }
}

LightConfigDialog::Vec3Editor LightConfigDialog::makeVec3Editor(QWidget*& container)
{
    container = new QWidget(m_formWidget);
    auto* hbox = new QHBoxLayout(container);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(4);

    constexpr std::array<const char*, 3> kAxisLabels{"x", "y", "z"};
    Vec3Editor editor{};
    for (int i = 0; i < 3; ++i) 
    {
        hbox->addWidget(new QLabel(QString::fromLatin1(kAxisLabels[i]), container));
        auto* spin = new QDoubleSpinBox(container);
        spin->setDecimals(4);
        spin->setRange(-kPosRange, kPosRange);
        spin->setSingleStep(0.1);
        hbox->addWidget(spin);
        editor[i] = spin;
    }
    return editor;
}

glm::vec3 LightConfigDialog::readVec3(const Vec3Editor& editor)
{
    return glm::vec3(static_cast<float>(editor[0]->value()),
                     static_cast<float>(editor[1]->value()),
                     static_cast<float>(editor[2]->value()));
}

void LightConfigDialog::writeVec3(const Vec3Editor& editor, const glm::vec3& v)
{
    for (int i = 0; i < 3; ++i) 
    {
        editor[i]->setValue(v[i]);
    }
}

bool LightConfigDialog::isSupported(LightType type) const
{
    return std::find(m_supportedTypes.begin(), m_supportedTypes.end(), type) != m_supportedTypes.end();
}

void LightConfigDialog::populateTypeCombo(LightType current)
{
    const bool wasLoading = m_loading;
    m_loading = true;
    m_typeCombo->clear();
    for (LightType t : m_supportedTypes) 
    {
        m_typeCombo->addItem(typeName(t), static_cast<int>(t));
    }
    if (!isSupported(current)) 
    {
        // Keep an existing light's type selectable so loading it doesn't
        // silently rewrite it to another type.
        m_typeCombo->addItem(tr("%1 (unsupported)").arg(typeName(current)), static_cast<int>(current));
    }
    m_loading = wasLoading;
}

QString LightConfigDialog::labelFor(int index, const LightConfig& light) const
{
    QString text = tr("%1 %2").arg(typeName(light.type)).arg(index + 1);
    if (!isSupported(light.type)) 
    {
        text += tr(" (unsupported)");
    }
    if (!light.enabled) 
    {
        text += tr(" (off)");
    }
    return text;
}

void LightConfigDialog::updateRowVisibility(LightType type)
{
    const bool spot = type == LightType::Spot;
    m_form->setRowVisible(m_positionRow, type != LightType::Directional);
    m_form->setRowVisible(m_directionRow, type != LightType::Point);
    m_form->setRowVisible(m_openingSpin, spot);
    m_form->setRowVisible(m_falloffSpin, spot);
    m_intensitySpin->setToolTip(type == LightType::Directional
                                    ? tr("Irradiance (W/m²)")
                                    : tr("Intensity (W/sr)"));
}

void LightConfigDialog::updateColorButton()
{
    if (m_row < 0) 
    {
        return;
    }
    const QColor c = toQColor(m_lights[m_row].color);
    m_colorButton->setText(c.name());
    m_colorButton->setStyleSheet(
        QStringLiteral("background-color: %1; color: %2;")
            .arg(c.name(), c.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white")));
}

void LightConfigDialog::updateEnabledState()
{
    const bool has = m_row >= 0;
    m_formWidget->setEnabled(has);
    m_removeButton->setEnabled(has);
}

void LightConfigDialog::addLight(LightType type)
{
    LightConfig light;
    light.type = type;
    m_lights.push_back(light);
    m_list->addItem(labelFor(static_cast<int>(m_lights.size()) - 1, light));
    m_list->setCurrentRow(static_cast<int>(m_lights.size()) - 1);
}

void LightConfigDialog::removeSelected()
{
    if (m_row < 0) 
    {
        return;
    }
    const int row = m_row;
    m_lights.erase(m_lights.begin() + row);
    m_row = -1; // the list reports the new current row after takeItem
    delete m_list->takeItem(row);
    for (size_t i = 0; i < m_lights.size(); ++i) 
    {
        m_list->item(static_cast<int>(i))->setText(labelFor(static_cast<int>(i), m_lights[i]));
    }
    onRowChanged(m_list->currentRow());
}

void LightConfigDialog::onRowChanged(int row)
{
    m_row = row;
    updateEnabledState();
    if (row < 0 || row >= static_cast<int>(m_lights.size())) 
    {
        m_row = -1;
        updateEnabledState();
        return;
    }

    const LightConfig& l = m_lights[row];
    m_loading = true;
    populateTypeCombo(l.type);
    m_enabledCheck->setChecked(l.enabled);
    m_typeCombo->setCurrentIndex(m_typeCombo->findData(static_cast<int>(l.type)));
    writeVec3(m_position, l.position);
    writeVec3(m_direction, l.direction);
    m_intensitySpin->setValue(l.intensity);
    m_openingSpin->setValue(l.openingAngleDegrees);
    m_falloffSpin->setValue(l.falloffAngleDegrees);
    m_loading = false;

    updateRowVisibility(l.type);
    updateColorButton();
}

void LightConfigDialog::onFieldEdited()
{
    if (m_loading || m_row < 0) 
    {
        return;
    }
    LightConfig& l = m_lights[m_row];
    l.enabled = m_enabledCheck->isChecked();
    l.type = static_cast<LightType>(m_typeCombo->currentData().toInt());
    l.position = readVec3(m_position);
    l.direction = readVec3(m_direction);
    l.intensity = static_cast<float>(m_intensitySpin->value());
    l.openingAngleDegrees = static_cast<float>(m_openingSpin->value());
    l.falloffAngleDegrees = std::min(static_cast<float>(m_falloffSpin->value()), l.openingAngleDegrees);

    updateRowVisibility(l.type);
    m_list->item(m_row)->setText(labelFor(m_row, l));
}

void LightConfigDialog::pickColor()
{
    if (m_row < 0) 
    {
        return;
    }
    const QColor chosen = QColorDialog::getColor(toQColor(m_lights[m_row].color), this, tr("Light Color"));
    if (!chosen.isValid()) 
    {
        return;
    }
    m_lights[m_row].color = glm::vec3(static_cast<float>(chosen.redF()),
                                      static_cast<float>(chosen.greenF()),
                                      static_cast<float>(chosen.blueF()));
    updateColorButton();
}

void LightConfigDialog::onApply()
{
    emit lightConfigsChanged(m_lights);
}

void LightConfigDialog::onAccepted()
{
    onApply();
    accept();
}

} // namespace vitrine
