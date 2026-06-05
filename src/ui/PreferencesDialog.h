#pragma once

#include <QDialog>

#include "UserPreferences.h"

class QCheckBox;
class QFormLayout;

namespace vitrine
{

/// @class PreferencesDialog
/// @brief User preferences dialog.
///
/// PreferencesDialog lets the user edit application-level preferences such as
/// the scene axis overlay visibility.
///
/// The dialog is seeded from a UserPreferences value. When the user accepts the
/// dialog, the edited preferences are emitted through preferencesChanged().
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param initial initial user preferences
    /// @param parent parent widget
    explicit PreferencesDialog(const UserPreferences& initial, QWidget* parent = nullptr);

    /// @brief Destructor
    ~PreferencesDialog() override = default;

    /// @brief Get the preferences currently shown in the widgets
    /// @return current user preferences
    UserPreferences preferences() const;

signals:
    /// @brief Signal emitted when the user accepts the dialog preferences
    /// @param preferences edited user preferences
    void preferencesChanged(const UserPreferences& preferences);

private slots:
    /// @brief Accept the dialog and emit the edited user preferences
    void onAccepted();

private:
    QFormLayout* m_form{nullptr};
    QCheckBox* m_showAxisOverlayCheck{nullptr};
};

} // namespace vitrine
