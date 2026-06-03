#pragma once

#include <QDialog>

#include <array>

#include "CameraConfig.h"

class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QWidget;

namespace vitrine
{

/// @brief Modal dialog for editing the active camera: its projection subtype
///        (perspective / orthographic) and view + projection properties
///        (eye / look-at / up, near / far, and either field of view or image
///        height). Seeded from a CameraConfig and emits cameraConfigChanged()
///        with the edited config when accepted.
class CameraConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CameraConfigDialog(const CameraConfig& initial, QWidget* parent = nullptr);
    ~CameraConfigDialog() override = default;

    /// @brief The configuration as currently shown in the widgets.
    CameraConfig config() const;

signals:
    /// @brief Emitted on accept with the edited configuration.
    void cameraConfigChanged(const CameraConfig& config);

private slots:
    void onTypeChanged(int comboIndex);
    void onAccepted();

private:
    // One row of three spin boxes editing a glm::vec3.
    struct Vec3Editor
    {
        std::array<QDoubleSpinBox*, 3> spins{nullptr, nullptr, nullptr};
    };
    Vec3Editor makeVec3Editor(const glm::vec3& value, QWidget* parent);
    static glm::vec3 readVec3(const Vec3Editor& editor);

    CameraType selectedType() const;

    QComboBox* m_typeCombo{nullptr};

    Vec3Editor m_eyeEditor;
    Vec3Editor m_centerEditor;
    Vec3Editor m_upEditor;

    QDoubleSpinBox* m_nearSpin{nullptr};
    QDoubleSpinBox* m_farSpin{nullptr};
    QDoubleSpinBox* m_fovYSpin{nullptr};
    QDoubleSpinBox* m_heightSpin{nullptr};

    // Owns the projection rows whose visibility toggles with the subtype
    // (toggled via setRowVisible on the field spin boxes).
    QFormLayout* m_form{nullptr};
};

} // namespace vitrine
