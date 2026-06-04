#pragma once

#include "AnariUtils.h"

#include <anari/anari.h>

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QSpinBox>

#include <memory>

class QComboBox;
class QFormLayout;
class QPushButton;
class QWidget;

namespace vitrine
{
/// @class AnariBackendDialog
/// @brief ANARI backend configuration dialog.
///
/// AnariBackendDialog lets the user select an ANARI library, device subtype,
/// renderer subtype, and renderer parameters discovered through ANARI
/// introspection.
///
/// The dialog owns a temporary ANARI library and device while it is open so it
/// can query backend capabilities without touching the live render thread. When
/// the user accepts the dialog, the selected configuration is emitted through
/// configurationChanged().
class AnariBackendDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Renderer parameter value selected in the dialog.
    struct ParamValue
    {
        QString name;
        int type{0}; // ANARIDataType as int
        QVariant value;
    };

    /// @brief Constructor
    /// @param initialLibrary initially selected ANARI library name
    /// @param initialDeviceSubtype initially selected ANARI device subtype
    /// @param initialRendererSubtype initially selected ANARI renderer subtype
    /// @param initialParameters initial renderer parameter values
    /// @param parent parent widget
    AnariBackendDialog(const QString& initialLibrary,
                       const QString& initialDeviceSubtype,
                       const QString& initialRendererSubtype,
                       const QVector<ParamValue>& initialParameters,
                       QWidget* parent = nullptr);

    /// @brief Destructor
    ~AnariBackendDialog() override;

    /// @brief Get the selected ANARI library name
    /// @return selected ANARI library name
    QString selectedLibrary() const { return m_selectedLibrary; }

    /// @brief Get the selected ANARI device subtype
    /// @return selected ANARI device subtype
    QString selectedDeviceSubtype() const { return m_selectedDeviceSubtype; }

    /// @brief Get the selected ANARI renderer subtype
    /// @return selected ANARI renderer subtype
    QString selectedRendererSubtype() const { return m_selectedRendererSubtype; }

    /// @brief Get the selected renderer parameters
    /// @return selected renderer parameter values
    QVector<ParamValue> selectedParameters() const { return m_selectedParameters; }

    /// @brief Convert a variant renderer parameter value to a color
    /// @param type ANARI data type stored in the variant
    /// @param v variant value to convert
    /// @return color represented by the variant, or black if conversion fails
    static QColor variantToColor(ANARIDataType type, const QVariant& v);

    /// @brief Update a push button to show a color swatch
    /// @param button button to update
    /// @param color color to display on the button
    static void setColorButtonSwatch(QPushButton* button, const QColor& color);

    /// @brief Create spin box editors for a numeric ANARI parameter
    /// @tparam T component type used by the ANARI parameter
    /// @param p renderer parameter metadata
    /// @param current the current value to set (if any)
    /// @param comps number of scalar components to expose
    /// @param parent parent Widget
    /// @return widget containing one or more spin boxes, or nullptr if comps is invalid
    template <typename T>
    QWidget* getSpinBoxContainer(const AnariRendererParameter &p, 
                                 const QVariant& current,
                                 const int comps, 
                                 QWidget* parent = nullptr)
    {
        if (comps < 1)
        {
            return nullptr;
        }

        bool notInt = true;
        
        if constexpr(std::is_same_v<T, int>)
        {
            notInt = false;
        }

        if (comps == 1)
        {
            if (notInt)
            {
                auto* spin = new QDoubleSpinBox(parent);
                spin->setDecimals(4);

                double min = -1e6;
                if (p.hasMinimum())
                {
                    min = *p.getMinimum<T>();
                }

                double max = 1e6;
                if (p.hasMaximum())
                {
                    max = *p.getMaximum<T>();
                }

                spin->setRange(min, max);
                spin->setSingleStep(0.1);

                if (current.isValid()) 
                {
                    spin->setValue(current.toDouble());
                }
                else 
                {
                    if (p.hasDefaultValue())
                    {
                        const double dv = *p.getDefaultValue<T>();
                        spin->setValue(dv);
                    }                
                }

                return spin;
            }
            else 
            {
                auto* spin = new QSpinBox(parent);

                const int min = p.hasMinimum() ? *p.getMinimum<int>() : -1000000;
                const int max = p.hasMaximum() ? *p.getMaximum<int>() : 1000000;
                spin->setRange(min,max);

                if (current.isValid()) 
                {
                    spin->setValue(current.toInt());
                }
                else 
                {
                    if (p.hasDefaultValue())
                    {
                        spin->setValue(*p.getDefaultValue<int>());
                    }
                }

                return spin;
            }
        }
        else
        {
            auto* container = new QWidget(parent);
            auto* hbox = new QHBoxLayout(container);
            hbox->setContentsMargins(0, 0, 0, 0);
            hbox->setSpacing(4);

            const auto list = current.toList();

            if (notInt)
            {
                std::array<double, 4> values{0.0, 0.0, 0.0, 0.0};

                if (current.isValid() && list.size() == comps)
                {
                    for (int i = 0; i < comps; ++i)
                    {
                        values[i] = list[i].toDouble();
                    }
                }
                else if (p.hasDefaultValue())
                {
                    const T* defaults = p.getDefaultValue<T>();
                    for (int i = 0; i < comps; ++i)
                    {
                        values[i] = defaults[i];
                    }
                }

                const T* minimum = p.hasMinimum() ? p.getMinimum<T>() : nullptr;
                const T* maximum = p.hasMaximum() ? p.getMaximum<T>() : nullptr;
                for (int i = 0; i < comps; ++i)
                {
                    auto* spin = new QDoubleSpinBox(container);
                    spin->setDecimals(4);
                    spin->setRange(minimum ? minimum[i] : -1e6,
                                    maximum ? maximum[i] : 1e6);
                    spin->setSingleStep(0.1);
                    spin->setValue(values[i]);
                    hbox->addWidget(spin);
                }
            }
            else
            {
                std::array<int, 4> values{0,0,0,0};

                if (current.isValid() && list.size() == comps)
                {
                    for (int i = 0; i < comps; ++i)
                    {
                        values[i] = list[i].toInt();
                    }
                }
                else if (p.hasDefaultValue())
                {
                    const int* defaults = p.getDefaultValue<int>();
                    for (int i = 0; i < comps; ++i)
                    {
                        values[i] = defaults[i];
                    }
                }

                const int min = p.hasMinimum() ? *p.getMinimum<int>() : -1000000;
                const int max = p.hasMaximum() ? *p.getMaximum<int>() : 1000000;
                for (int i = 0; i < comps; ++i)
                {
                    auto* spin = new QSpinBox(container);
                    spin->setRange(min,max);
                    spin->setValue(values[i]);
                    hbox->addWidget(spin);
                }
            }

            return container;
        }
    }

signals:
    /// @brief Signal emitted when the user accepts the dialog configuration
    /// @param library selected ANARI library name
    /// @param deviceSubtype selected ANARI device subtype
    /// @param rendererSubtype selected ANARI renderer subtype
    /// @param parameters selected renderer parameter values
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
    void buildParameterPanel(const int);
    void releaseProbe();
    QWidget* makeWidgetForParameter(const AnariRendererParameter& p, const QVariant& current);
    QVariant readWidget(int type, QWidget* editor) const;

    QPushButton* createColorButton(QWidget*, ANARIDataType type, const QVariant&);
    
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
