#pragma once

#include <anari/anari.h>

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

#include "LogLevel.h"

namespace vitrine
{

/// @class AnariStatusSink
/// @brief ANARI status callback bridge.
///
/// AnariStatusSink translates ANARI status callback messages into Qt-friendly
/// log messages. It stores the application callback as a std::function and
/// exposes the C callback pointer and opaque user pointer required by
/// anariLoadLibrary().
///
/// The caller owns the sink and must keep it alive for the lifetime of the
/// loaded ANARI library using the callback.
class AnariStatusSink
{
public:
    /// @brief Function type used to receive ANARI status messages.
    using Sink = std::function<void(LogLevel, QString)>;

    /// @brief Constructor
    /// @param sink callback invoked for translated ANARI status messages
    explicit AnariStatusSink(Sink sink);

    /// @brief Get the C-style callback for anariLoadLibrary()
    /// @return ANARI status callback function pointer
    ANARIStatusCallback callback() const { return &AnariStatusSink::dispatch; }

    /// @brief Get the opaque user pointer for anariLoadLibrary()
    /// @return user pointer passed back to the status callback
    const void* userData() const { return this; }

private:
    static void dispatch(const void* userPtr,
                         ANARIDevice device,
                         ANARIObject source,
                         ANARIDataType sourceType,
                         ANARIStatusSeverity severity,
                         ANARIStatusCode code,
                         const char* message);

    Sink m_sink;
};

/// @class AnariRendererParameter
/// @brief Renderer parameter metadata discovered through ANARI introspection.
///
/// AnariRendererParameter stores the name, type, description, accepted string
/// values, and optional min/max/default metadata for a single renderer
/// parameter exposed by an ANARI backend.
///
/// The min, max, and default pointers refer to metadata owned by the ANARI
/// implementation. This class does not take ownership of those values.
class AnariRendererParameter
{
public:
    /// @brief Default constructor
    AnariRendererParameter() = default;

    /// @brief Move constructor
    /// @param other renderer parameter metadata to move from
    AnariRendererParameter(AnariRendererParameter &&other);

    /// @brief Copy constructor
    AnariRendererParameter(const AnariRendererParameter &) = delete;

    /// @brief Destructor
    ~AnariRendererParameter() = default;

    /// @brief Move assignment operator
    /// @param other renderer parameter metadata to move from
    /// @return reference to this renderer parameter
    AnariRendererParameter &operator=(AnariRendererParameter &&other);

    /// @brief Copy assignment operator
    AnariRendererParameter &operator=(const AnariRendererParameter &) = delete;

    /// @brief Set the parameter name
    /// @param n UTF-8 parameter name returned by ANARI
    void setName(const char *n) { m_name = QString::fromUtf8(n); }

    /// @brief Get the parameter name
    /// @return parameter name
    QString getName() const { return m_name; }

    /// @brief Set the parameter data type
    /// @param t ANARI data type for this parameter
    void setType(ANARIDataType t) { m_type = t;}

    /// @brief Get the parameter data type
    /// @return ANARI data type for this parameter
    ANARIDataType getType() const { return m_type; }

    /// @brief Set the parameter description
    /// @param description UTF-8 description returned by ANARI
    void setDescription(const char *);

    /// @brief Get the parameter description
    /// @return parameter description, or an empty string if none is available
    QString getDescription() const { return m_description; }

    /// @brief Set the accepted string values for this parameter
    /// @param v null-terminated list of UTF-8 strings returned by ANARI
    void setAcceptedValues(const char **);

    /// @brief Get the accepted string values for this parameter
    /// @return accepted string values, or an empty vector if unrestricted
    QVector<QString> getAcceptedValues() const { return m_acceptedValues; }

    /// @brief Set the minimum parameter value metadata
    /// @param min pointer returned by ANARI for the minimum value
    void setMinimum(const void *min) { m_minimum = min; }

    /// @brief Get the minimum parameter value metadata
    /// @tparam T component type expected by the caller
    /// @return typed pointer to the minimum value, or nullptr if unavailable
    template <typename T>
    const T *getMinimum() const
    {
        return static_cast<const T *>(m_minimum);
    }

    /// @brief Set the maximum parameter value metadata
    /// @param max pointer returned by ANARI for the maximum value
    void setMaximum(const void *max) { m_maximum = max; }

    /// @brief Get the maximum parameter value metadata
    /// @tparam T component type expected by the caller
    /// @return typed pointer to the maximum value, or nullptr if unavailable
    template <typename T>
    const T *getMaximum() const
    {
        return static_cast<const T *>(m_maximum);
    }

    /// @brief Set the default parameter value metadata
    /// @param dv pointer returned by ANARI for the default value
    void setDefaultValue(const void *dv) { m_defaultValue = dv; }

    /// @brief Get the default parameter value metadata
    /// @tparam T component type expected by the caller
    /// @return typed pointer to the default value, or nullptr if unavailable
    template <typename T>
    const T *getDefaultValue() const
    {
        return static_cast<const T *>(m_defaultValue);
    }

    /// @brief Check whether minimum value metadata is available
    /// @return true if a minimum value pointer is set, false otherwise
    bool hasMinimum() const { bool val = (m_minimum != NULL && m_minimum != nullptr); return val; }

    /// @brief Check whether maximum value metadata is available
    /// @return true if a maximum value pointer is set, false otherwise
    bool hasMaximum() const { bool val = (m_maximum != NULL && m_maximum != nullptr); return val; }

    /// @brief Check whether default value metadata is available
    /// @return true if a default value pointer is set, false otherwise
    bool hasDefaultValue() const
    {
        bool val = (m_defaultValue != NULL && m_defaultValue != nullptr); return val;
    }

private:
    QString m_name;
    ANARIDataType m_type;
    QString m_description;
    QVector<QString> m_acceptedValues;

    const void *m_minimum{nullptr};
    const void *m_maximum{nullptr};
    const void *m_defaultValue{nullptr};
};

/// @namespace AnariUtils
/// @brief ANARI backend and renderer introspection helpers.
namespace AnariUtils
{
    /// @brief Enumerate ANARI backend libraries available to the UI
    /// @return ANARI library names discovered from built-ins and library paths
    QStringList enumerateBackendLibraries();

    /// @brief Enumerate device subtypes advertised by an ANARI library
    /// @param library loaded ANARI library to query
    /// @return device subtype names exposed by the library
    QStringList enumerateDeviceSubtypes(ANARILibrary library);

    /// @brief Enumerate renderer subtypes advertised by an ANARI device
    /// @param device ANARI device to query
    /// @return renderer subtype names exposed by the device
    QStringList enumerateRendererSubtypes(ANARIDevice device);

    /// @brief Enumerate renderer parameters for a renderer subtype
    /// @param device ANARI device to query
    /// @param rendererSubtype renderer subtype name to inspect
    /// @return renderer parameter metadata exposed by the subtype
    std::vector<AnariRendererParameter> enumerateRendererParameters(ANARIDevice device, const QString& rendererSubtype);
} // namespace AnariUtils

} // namespace vitrine
