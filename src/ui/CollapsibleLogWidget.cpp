#include "CollapsibleLogWidget.h"

#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QMenu>
#include <QScrollBar>
#include <QFile>
#include <QTextStream>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
CollapsibleLogWidget::CollapsibleLogWidget(const QString &title, int expandedHeight, QWidget* parent) 
    : QWidget(parent), m_expandedHeight(expandedHeight)
{
    setObjectName("CollapsibleLogWidget");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setText(title);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setArrowType(Qt::RightArrow);
    m_toggleButton->setObjectName("logToggleButton");
    m_toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(m_toggleButton);

    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setLineWrapMode(QTextEdit::WidgetWidth);
    m_logText->setMinimumHeight(0);
    m_logText->setMaximumHeight(0); // Start collapsed
    m_logText->setObjectName("logText");
    m_logText->setContextMenuPolicy(Qt::CustomContextMenu);

    // Auto-scroll to bottom on new log messages
    connect(m_logText, &QTextEdit::textChanged, [this]() {
        QScrollBar* scrollBar = m_logText->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });

    // Custom context menu
    connect(m_logText, &QTextEdit::customContextMenuRequested, this, &CollapsibleLogWidget::customContextMenuRequested);
    mainLayout->addWidget(m_logText);

    // Setup Animation
    m_animationGroup = new QParallelAnimationGroup(this);
    auto* heightAnimation = new QPropertyAnimation(m_logText, "maximumHeight");
    heightAnimation->setDuration(300);
    heightAnimation->setEasingCurve(QEasingCurve::InOutQuart);
    m_animationGroup->addAnimation(heightAnimation);

    connect(m_toggleButton, &QToolButton::toggled, this, &CollapsibleLogWidget::toggle);

    applyStyleSheet();
}

//----------------------------------------------------------------------------------
void CollapsibleLogWidget::applyStyleSheet()
{
    QFile styleFile(":/log_widget_style.qss");
    
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&styleFile);
    setStyleSheet(stream.readAll());
}

//----------------------------------------------------------------------------------
void CollapsibleLogWidget::toggle(bool expanded) 
{
    m_toggleButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    auto* heightAnimation = static_cast<QPropertyAnimation*>(m_animationGroup->animationAt(0));
    
    if(expanded) 
    {
        heightAnimation->setStartValue(0);
        heightAnimation->setEndValue(m_expandedHeight);
    } 
    else 
    {
        heightAnimation->setStartValue(m_expandedHeight);
        heightAnimation->setEndValue(0);
    }

    m_animationGroup->start();
}

//----------------------------------------------------------------------------------
void CollapsibleLogWidget::customContextMenuRequested(const QPoint& pos) 
{
    QMenu contextMenu(this);
    // Clear Action
    QAction* clearAction = contextMenu.addAction("Clear Log");
    connect(clearAction, &QAction::triggered, m_logText, &QTextEdit::clear);
    
    // Copy Action
    QAction* copyAction = contextMenu.addAction("Copy Log");
    connect(copyAction, &QAction::triggered, [this]() {
        m_logText->selectAll();
        m_logText->copy();
    });

    contextMenu.exec(m_logText->mapToGlobal(pos));
}

//----------------------------------------------------------------------------------
void CollapsibleLogWidget::appendLogMessage(const QString &message, LogLevel level)
{
    if (!m_logText) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString levelStr = 
        (level == LogLevel::Info) ? "INFO" :
        (level == LogLevel::Warning) ? "WARNING" :
        (level == LogLevel::Error) ? "ERROR" : "UNKNOWN";

    // Color text based on log level
    if (level == LogLevel::Warning) {
        m_logText->setTextColor(Qt::darkYellow);
    } else if (level == LogLevel::Error) {
        m_logText->setTextColor(Qt::red);
    } else {
        m_logText->setTextColor(Qt::black);
    }

    m_logText->append(QString("%1 - [%2] %3").arg(timestamp, levelStr, message));
}
} // namespace myvulkan