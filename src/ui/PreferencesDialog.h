#pragma once

#include <QDialog>

#include "UserPreferences.h"

class QCheckBox;
class QFormLayout;

namespace vitrine
{

/// @brief Modal dialog for editing user preferences. Seeded from a
///        UserPreferences and emits preferencesChanged() with the edited values
///        when accepted. The form layout is the expansion surface: future
///        preferences add a widget + row here and a field in UserPreferences.
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(const UserPreferences& initial, QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

    /// @brief The preferences as currently shown in the widgets.
    UserPreferences preferences() const;

signals:
    /// @brief Emitted on accept with the edited preferences.
    void preferencesChanged(const UserPreferences& preferences);

private slots:
    void onAccepted();

private:
    QFormLayout* m_form{nullptr};
    QCheckBox* m_showAxisOverlayCheck{nullptr};
};

} // namespace vitrine
