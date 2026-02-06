#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QParallelAnimationGroup>
#include <QScrollArea>

namespace myvulkan 
{
/// @brief Log levels for messages appended to the log panel.
enum class LogLevel 
{
    Info,
    Warning,
    Error
};

/// @brief A collapsible log widget that can be expanded or collapsed by the user.
class CollapsibleLogWidget : public QWidget 
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param title the title of the log widget.
    /// @param expandedHeight the height of the widget when expanded.
    /// @param parent the parent widget.
    explicit CollapsibleLogWidget(const QString &title, int expandedHeight=200, QWidget* parent = nullptr);
    
    /// @brief Destructor
    ~CollapsibleLogWidget() = default;

    /// @brief Appends a log message to the log text area.
    /// @param message The log message to append.
    /// @param level The severity level of the log message.
    void appendLogMessage(const QString &message, LogLevel level = LogLevel::Info);

private slots:
    void toggle(bool expanded);
    void customContextMenuRequested(const QPoint& pos);

private:
    void applyStyleSheet();

    QToolButton* m_toggleButton = nullptr;
    QTextEdit* m_logText = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QParallelAnimationGroup* m_animationGroup = nullptr;
    int m_expandedHeight = 200;
};


} // namespace myvulkan