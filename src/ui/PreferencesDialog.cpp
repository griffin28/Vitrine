#include "PreferencesDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace vitrine
{

PreferencesDialog::PreferencesDialog(const UserPreferences& initial, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    m_form = new QFormLayout();
    mainLayout->addLayout(m_form);

    // First preference: toggle the camera axis-overlay gizmo.
    m_showAxisOverlayCheck = new QCheckBox(this);
    m_showAxisOverlayCheck->setChecked(initial.showAxisOverlay);
    m_form->addRow(tr("Show Axis Overlay"), m_showAxisOverlayCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &PreferencesDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

UserPreferences PreferencesDialog::preferences() const
{
    UserPreferences prefs;
    prefs.showAxisOverlay = m_showAxisOverlayCheck->isChecked();
    return prefs;
}

void PreferencesDialog::onAccepted()
{
    emit preferencesChanged(preferences());
    accept();
}

} // namespace vitrine
