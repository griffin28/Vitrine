#pragma once

#include <QDialog>

#include <array>
#include <vector>

#include "LightConfig.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QListWidget;
class QPushButton;
class QWidget;

namespace vitrine
{

/// @class LightConfigDialog
/// @brief Scene light editor.
///
/// Shows the list of configured lights on the left and a per-type property
/// form on the right. Edits are made on a working copy; when the user accepts
/// the dialog the full list is emitted through lightConfigsChanged().
class LightConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param initial lights to edit
    /// @param parent parent widget
    explicit LightConfigDialog(const std::vector<LightConfig>& initial, QWidget* parent = nullptr);

    /// @brief Destructor
    ~LightConfigDialog() override = default;

    /// @brief Get the light list as currently edited
    /// @return working copy of the lights
    std::vector<LightConfig> configs() const { return m_lights; }

    /// @brief Generous world-space bounds for position/direction components
    static constexpr double kPosRange = 1.0e6;

signals:
    /// @brief Signal emitted when the user accepts the dialog
    /// @param lights edited light configurations
    void lightConfigsChanged(const std::vector<LightConfig>& lights);

private slots:
    /// @brief Append a new light of the given type
    /// @param type light type to add
    void addLight(LightType type);

    /// @brief Remove the selected light
    void removeSelected();

    /// @brief Load the form from the newly selected light
    /// @param row selected list row, or -1
    void onRowChanged(int row);

    /// @brief Copy the form into the selected light and refresh its list label
    void onFieldEdited();

    /// @brief Choose the selected light's color via a color dialog
    void pickColor();

    /// @brief Emit the edited lights without closing the dialog
    void onApply();

    /// @brief Apply the edited lights and close the dialog
    void onAccepted();

private:
    using Vec3Editor = std::array<QDoubleSpinBox*, 3>;

    /// @brief Create a row of three spin boxes editing a vec3
    /// @param container receives the row widget
    /// @return the three spin boxes
    Vec3Editor makeVec3Editor(QWidget*& container);

    /// @brief Read a vec3 from spin boxes
    static glm::vec3 readVec3(const Vec3Editor& editor);

    /// @brief Write a vec3 into spin boxes
    static void writeVec3(const Vec3Editor& editor, const glm::vec3& v);

    /// @brief Show only the form rows relevant to a light type
    void updateRowVisibility(LightType type);

    /// @brief List label for a light
    QString labelFor(int index, const LightConfig& light) const;

    /// @brief Refresh the color button swatch
    void updateColorButton();

    /// @brief Enable/disable the form and remove button for the selection
    void updateEnabledState();

    std::vector<LightConfig> m_lights;
    int m_row{-1};
    bool m_loading{false};

    QListWidget* m_list{nullptr};
    QPushButton* m_removeButton{nullptr};
    QWidget* m_formWidget{nullptr};
    QFormLayout* m_form{nullptr};

    QCheckBox* m_enabledCheck{nullptr};
    QComboBox* m_typeCombo{nullptr};
    QPushButton* m_colorButton{nullptr};
    QWidget* m_positionRow{nullptr};
    QWidget* m_directionRow{nullptr};
    Vec3Editor m_position{};
    Vec3Editor m_direction{};
    QDoubleSpinBox* m_intensitySpin{nullptr};
    QDoubleSpinBox* m_openingSpin{nullptr};
    QDoubleSpinBox* m_falloffSpin{nullptr};
};

} // namespace vitrine
