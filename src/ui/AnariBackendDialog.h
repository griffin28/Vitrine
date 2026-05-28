#pragma once

#include <anari/anari.h>

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVector>

#include <memory>

class QComboBox;
class QFormLayout;
class QPushButton;
class QWidget;

namespace vitrine
{

class AnariStatusSink;

/// @brief Modal dialog that lets the user pick an ANARI backend library +
///        device subtype + renderer subtype, and tweak the renderer
///        parameters discovered via ANARI introspection.
///
/// The dialog loads its own throwaway ANARI library/device while it is open
/// (so introspection works without disturbing the real render thread).
/// When the user accepts, it emits configurationChanged() with the final
/// selections; the main window forwards those to the live AnariRenderer.
class AnariBackendDialog : public QDialog
{
    Q_OBJECT

public:
    struct ParamValue
    {
        QString name;
        int type{0}; // ANARIDataType as int
        QVariant value;
    };

    AnariBackendDialog(const QString& initialLibrary,
                       const QString& initialDeviceSubtype,
                       const QString& initialRendererSubtype,
                       const QVector<ParamValue>& initialParameters,
                       QWidget* parent = nullptr);
    ~AnariBackendDialog() override;

    QString selectedLibrary() const { return m_selectedLibrary; }
    QString selectedDeviceSubtype() const { return m_selectedDeviceSubtype; }
    QString selectedRendererSubtype() const { return m_selectedRendererSubtype; }
    QVector<ParamValue> selectedParameters() const { return m_selectedParameters; }

signals:
    void configurationChanged(const QString& library,
                              const QString& deviceSubtype,
                              const QString& rendererSubtype,
                              const QVector<ParamValue>& parameters);

private slots:
    void onLibraryChanged(int comboIndex);
    void onDeviceSubtypeChanged(int comboIndex);
    void onRendererSubtypeChanged(int comboIndex);
    void onAccepted();

private:
    void buildParameterPanel();
    void releaseProbe();
    QWidget* makeEditorForParameter(int type, const QString& name, const QVariant& current);
    QVariant readEditor(int type, QWidget* editor) const;

    // UI
    QComboBox* m_libraryCombo{nullptr};
    QComboBox* m_deviceSubtypeCombo{nullptr};
    QComboBox* m_rendererSubtypeCombo{nullptr};
    QFormLayout* m_parameterForm{nullptr};
    QWidget* m_parameterContainer{nullptr};

    // Probe state (rebuilt every time the library changes).
    ANARILibrary m_probeLibrary{nullptr};
    ANARIDevice m_probeDevice{nullptr};
    std::unique_ptr<AnariStatusSink> m_probeStatusSink;

    // Working selections (committed on accept).
    QString m_selectedLibrary;
    QString m_selectedDeviceSubtype;
    QString m_selectedRendererSubtype;
    QVector<ParamValue> m_selectedParameters;

    // Editors currently shown, keyed by parameter name; each carries its
    // ANARI type alongside so readEditor can deserialise on accept.
    struct EditorEntry
    {
        QString name;
        int type{0};
        QWidget* editor{nullptr};
    };
    QVector<EditorEntry> m_editors;

    // Initial parameter values seeded by the caller, used to pre-populate
    // editors when the introspected parameter list matches.
    QVector<ParamValue> m_initialParameters;
};

} // namespace vitrine
