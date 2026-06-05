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

/// @class CameraConfigDialog
/// @brief Camera configuration dialog.
///
/// CameraConfigDialog lets the user edit the active camera projection subtype,
/// view vectors, clip planes, and projection-specific settings.
///
/// The dialog is seeded from a CameraConfig. When the user accepts the dialog,
/// the edited configuration is emitted through cameraConfigChanged().
class CameraConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param initial initial camera configuration
    /// @param parent parent widget
    explicit CameraConfigDialog(const CameraConfig& initial, QWidget* parent = nullptr);

    /// @brief Destructor
    ~CameraConfigDialog() override = default;

    /// @brief Get the configuration currently shown in the widgets
    /// @return current camera configuration
    CameraConfig config() const;

    /// @brief Generous world-space bounds for position, look-at, and up components
    static constexpr double kPosRange = 1.0e6;

signals:
    /// @brief Signal emitted when the user accepts the dialog configuration
    /// @param config edited camera configuration
    void cameraConfigChanged(const CameraConfig& config);

private slots:
    /// @brief Update projection-specific controls after the camera type changes
    /// @param comboIndex selected combo box index
    void onTypeChanged(int comboIndex);

    /// @brief Accept the dialog and emit the edited camera configuration
    void onAccepted();

private:
    /// @brief One row of three spin boxes editing a glm::vec3
    struct Vec3Editor
    {
        std::array<QDoubleSpinBox*, 3> spins{nullptr, nullptr, nullptr};
    };

    /// @brief Create spin box editors for a 3D vector
    /// @param value initial vector value
    /// @param parent parent widget
    /// @return editor containing one spin box per vector component
    Vec3Editor makeVec3Editor(const glm::vec3& value, QWidget* parent);

    /// @brief Read a 3D vector from spin box editors
    /// @param editor vector editor to read
    /// @return vector represented by the editor values
    static glm::vec3 readVec3(const Vec3Editor& editor);

    /// @brief Get the selected camera projection type
    /// @return selected camera type
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
