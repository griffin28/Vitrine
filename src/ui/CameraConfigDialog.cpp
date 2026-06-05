#include "CameraConfigDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <array>

namespace vitrine
{

CameraConfigDialog::CameraConfigDialog(const CameraConfig& initial, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Camera Configuration"));
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    m_form = new QFormLayout();
    mainLayout->addLayout(m_form);

    // Projection subtype.
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Perspective"), static_cast<int>(CameraType::Perspective));
    m_typeCombo->addItem(tr("Orthographic"), static_cast<int>(CameraType::Orthographic));
    m_typeCombo->setCurrentIndex(initial.type == CameraType::Orthographic ? 1 : 0);
    m_form->addRow(tr("Type: "), m_typeCombo);

    // View vectors.
    m_eyeEditor = makeVec3Editor(initial.eye, this);
    m_centerEditor = makeVec3Editor(initial.center, this);
    m_upEditor = makeVec3Editor(initial.up, this);
    m_form->addRow(tr("Position: "), m_eyeEditor.spins[0]->parentWidget());
    m_form->addRow(tr("Look At: "), m_centerEditor.spins[0]->parentWidget());
    m_form->addRow(tr("Up: "), m_upEditor.spins[0]->parentWidget());

    // Clip planes.
    m_nearSpin = new QDoubleSpinBox(this);
    m_nearSpin->setDecimals(4);
    m_nearSpin->setRange(1.0e-4, kPosRange);
    m_nearSpin->setSingleStep(0.01);
    m_nearSpin->setValue(initial.nearClip);
    m_form->addRow(tr("Near: "), m_nearSpin);

    m_farSpin = new QDoubleSpinBox(this);
    m_farSpin->setDecimals(2);
    m_farSpin->setRange(1.0e-3, kPosRange);
    m_farSpin->setSingleStep(1.0);
    m_farSpin->setValue(initial.farClip);
    m_form->addRow(tr("Far: "), m_farSpin);

    // Perspective-only: vertical field of view (degrees).
    m_fovYSpin = new QDoubleSpinBox(this);
    m_fovYSpin->setDecimals(2);
    m_fovYSpin->setRange(1.0, 179.0);
    m_fovYSpin->setSingleStep(1.0);
    m_fovYSpin->setSuffix(QStringLiteral("°"));
    m_fovYSpin->setValue(initial.fovYDegrees);
    m_form->addRow(tr("Field of View: "), m_fovYSpin);

    // Orthographic-only: image-plane height (world units).
    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setDecimals(4);
    m_heightSpin->setRange(1.0e-4, kPosRange);
    m_heightSpin->setSingleStep(0.1);
    m_heightSpin->setValue(initial.height);
    m_form->addRow(tr("Height: "), m_heightSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CameraConfigDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_typeCombo, &QComboBox::currentIndexChanged,
            this, &CameraConfigDialog::onTypeChanged);

    onTypeChanged(m_typeCombo->currentIndex());
}

CameraConfigDialog::Vec3Editor CameraConfigDialog::makeVec3Editor(const glm::vec3& value, QWidget* parent)
{
    auto* container = new QWidget(parent);
    auto* hbox = new QHBoxLayout(container);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(4);

    constexpr std::array<const char*, 3> kAxisLabels{"x", "y", "z"};

    Vec3Editor editor;
    for (int i = 0; i < 3; ++i) 
    {
        hbox->addWidget(new QLabel(QString::fromLatin1(kAxisLabels[i]), container));

        auto* spin = new QDoubleSpinBox(container);
        spin->setDecimals(4);
        spin->setRange(-kPosRange, kPosRange);
        spin->setSingleStep(0.1);
        spin->setValue(value[i]);
        hbox->addWidget(spin);
        editor.spins[i] = spin;
    }
    return editor;
}

glm::vec3 CameraConfigDialog::readVec3(const Vec3Editor& editor)
{
    return glm::vec3(static_cast<float>(editor.spins[0]->value()),
                     static_cast<float>(editor.spins[1]->value()),
                     static_cast<float>(editor.spins[2]->value()));
}

CameraType CameraConfigDialog::selectedType() const
{
    return static_cast<CameraType>(m_typeCombo->currentData().toInt());
}

void CameraConfigDialog::onTypeChanged(int /*comboIndex*/)
{
    const bool perspective = selectedType() == CameraType::Perspective;
    m_form->setRowVisible(m_fovYSpin, perspective);
    m_form->setRowVisible(m_heightSpin, !perspective);
}

CameraConfig CameraConfigDialog::config() const
{
    CameraConfig cfg;
    cfg.type = selectedType();
    cfg.eye = readVec3(m_eyeEditor);
    cfg.center = readVec3(m_centerEditor);
    cfg.up = readVec3(m_upEditor);
    cfg.nearClip = static_cast<float>(m_nearSpin->value());
    cfg.farClip = static_cast<float>(m_farSpin->value());
    cfg.fovYDegrees = static_cast<float>(m_fovYSpin->value());
    cfg.height = static_cast<float>(m_heightSpin->value());
    return cfg;
}

void CameraConfigDialog::onAccepted()
{
    emit cameraConfigChanged(config());
    accept();
}

} // namespace vitrine
