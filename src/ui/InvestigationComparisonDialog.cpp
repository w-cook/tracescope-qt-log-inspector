#include "InvestigationComparisonDialog.h"

#include <algorithm>
#include <cmath>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include "../analysis/InvestigationCadenceAnalyzer.h"
#include "../workspace/InvestigationSession.h"
#include "../workspace/InvestigationWorkspace.h"

#include "InterfaceScale.h"

namespace
{
QString sessionDisplayText(
    const InvestigationSession &session
    )
{
    const InvestigationSessionSourceMetadata &metadata =
        session.sourceMetadata();

    QString name =
        metadata
            .sourceName
            .trimmed();

    if (name.isEmpty()) {
        name =
            session.id();
    }

    return name;
}

std::optional<InvestigationComparisonTimeRange>
activeRangeFor(
    const InvestigationSession *session
    )
{
    if (session == nullptr) {
        return std::nullopt;
    }

    const auto *proxy =
        session->investigationController()
            ->proxyModel();

    InvestigationComparisonTimeRange range;

    range.startTime =
        proxy->timeRangeStart();

    range.endTime =
        proxy->timeRangeEnd();

    return range.isValid()
               ? std::make_optional(range)
               : std::nullopt;
}

bool containsRecordsInRange(
    const InvestigationSession &session,
    const InvestigationComparisonTimeRange &range
    )
{
    const auto &records =
        session.investigationController()
            ->allRecords();

    return std::any_of(
        records.cbegin(),
        records.cend(),
        [&range](const InvestigationRecord &record) {
            if (!record.timestamp.has_value()
                || !record.timestamp->isValid()) {
                return false;
            }

            const QDateTime &timestamp =
                *record.timestamp;

            return (!range.startTime.has_value()
                    || timestamp >= *range.startTime)
                   && (!range.endTime.has_value()
                       || timestamp <= *range.endTime);
        }
        );
}
}

InvestigationComparisonDialog::
    InvestigationComparisonDialog(
        InvestigationWorkspace *workspace,
        const QString &initialBaselineSessionId,
        const QString &initialComparisonSessionId,
        QWidget *parent
        )
    : QDialog(parent),
    m_workspace(workspace),
    m_baselineCombo(
        new QComboBox(this)
        ),
    m_baselineTimeRangeCheck(
        new QCheckBox(
            tr("Use this session's active time range"),
            this
            )
        ),
    m_comparisonCombo(
        new QComboBox(this)
        ),
    m_comparisonTimeRangeCheck(
        new QCheckBox(
            tr("Use this session's active time range"),
            this
            )
        ),
    m_burstGroup(
        new QGroupBox(
            tr("Burst Comparison"),
            this
            )
        ),
    m_windowSpin(
        new QDoubleSpinBox(
            m_burstGroup
            )
        ),
    m_mergeGapSpin(
        new QDoubleSpinBox(
            m_burstGroup
            )
        ),
    m_elevatedSpin(
        new QSpinBox(
            m_burstGroup
            )
        ),
    m_errorCriticalSpin(
        new QSpinBox(
            m_burstGroup
            )
        ),
    m_validationLabel(
        new QLabel(this)
        ),
    m_buttons(
        new QDialogButtonBox(
            QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel,
            this
            )
        )
{
    setWindowTitle(
        tr("Compare Investigation Sessions")
        );

    setMinimumWidth(
        InterfaceScale::pixels(
            520,
            this
            )
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        InterfaceScale::pixels(
            8,
            this
            )
        );

    auto *description =
        new QLabel(
            tr(
                "Compare two imported sessions. "
                "By default, each side uses its complete "
                "imported record population. Optionally, "
                "capture either session's active time range. "
                "Other active filters are ignored. "
                "Every delta is calculated as "
                "Comparison − Baseline."
                ),
            this
            );

    description->setWordWrap(
        true
        );

    layout->addWidget(
        description
        );

    /*
     * ---------------------------------------------------------
     * Source sessions
     * ---------------------------------------------------------
     */

    auto *sessionsGroup =
        new QGroupBox(
            tr("Sessions"),
            this
            );

    auto *sessionsLayout =
        new QFormLayout(
            sessionsGroup
            );

    sessionsLayout->addRow(
        tr("Baseline:"),
        m_baselineCombo
        );

    sessionsLayout->addRow(
        QString(),
        m_baselineTimeRangeCheck
        );

    sessionsLayout->addRow(
        tr("Comparison:"),
        m_comparisonCombo
        );

    sessionsLayout->addRow(
        QString(),
        m_comparisonTimeRangeCheck
        );

    m_baselineTimeRangeCheck->setObjectName(
        QStringLiteral("baselineTimeRangeCheck")
        );

    m_comparisonTimeRangeCheck->setObjectName(
        QStringLiteral("comparisonTimeRangeCheck")
        );

    auto *swapButton =
        new QPushButton(
            tr("Swap Baseline / Comparison"),
            sessionsGroup
            );

    swapButton->setObjectName(
        QStringLiteral("swapSessionsButton")
        );

    sessionsLayout->addRow(
        QString(),
        swapButton
        );

    connect(
        swapButton,
        &QPushButton::clicked,
        this,
        [this]() {
            const int baselineIndex =
                m_baselineCombo->currentIndex();

            const int comparisonIndex =
                m_comparisonCombo->currentIndex();

            if (baselineIndex < 0
                || comparisonIndex < 0
                || baselineIndex
                       == comparisonIndex) {
                return;
            }

            const bool baselineScoped =
                m_baselineTimeRangeCheck->isChecked();

            const bool comparisonScoped =
                m_comparisonTimeRangeCheck->isChecked();

            /*
             * Avoid recalculating shared burst defaults
             * twice while the two selections are in an
             * intermediate swapped state.
             */
            const QSignalBlocker baselineBlocker(
                m_baselineCombo
                );

            const QSignalBlocker comparisonBlocker(
                m_comparisonCombo
                );

            m_baselineCombo->setCurrentIndex(
                comparisonIndex
                );

            m_comparisonCombo->setCurrentIndex(
                baselineIndex
                );

            const QSignalBlocker baselineRangeBlocker(
                m_baselineTimeRangeCheck
                );

            const QSignalBlocker comparisonRangeBlocker(
                m_comparisonTimeRangeCheck
                );

            m_baselineTimeRangeCheck->setChecked(
                comparisonScoped
                );

            m_comparisonTimeRangeCheck->setChecked(
                baselineScoped
                );

            refreshTimeRangeOptions();
            updateValidation();
            applySharedBurstDefaults();
        }
        );

    layout->addWidget(
        sessionsGroup
        );

    /*
     * ---------------------------------------------------------
     * Optional shared burst analysis
     * ---------------------------------------------------------
     */

    m_burstGroup->setCheckable(
        true
        );

    m_burstGroup->setChecked(
        true
        );

    auto *burstLayout =
        new QVBoxLayout(
            m_burstGroup
            );

    auto *burstDescription =
        new QLabel(
            tr(
                "Shared automatic recommendations are "
                "derived from the selected record populations. "
                "The same explicit burst-detection settings "
                "are applied to both sides. You may adjust "
                "them before creating the comparison."
                ),
            m_burstGroup
            );

    burstDescription->setWordWrap(
        true
        );

    burstLayout->addWidget(
        burstDescription
        );

    auto *burstForm =
        new QFormLayout();

    const BurstDetectionSettings defaults;

    m_windowSpin->setDecimals(
        3
        );

    m_windowSpin->setRange(
        0.001,
        7.0 * 24.0 * 60.0 * 60.0
        );

    m_windowSpin->setSuffix(
        tr(" s")
        );

    m_windowSpin->setValue(
        static_cast<double>(
            defaults.windowMilliseconds
            )
        / 1000.0
        );

    m_mergeGapSpin->setDecimals(
        3
        );

    m_mergeGapSpin->setRange(
        0.0,
        7.0 * 24.0 * 60.0 * 60.0
        );

    m_mergeGapSpin->setSuffix(
        tr(" s")
        );

    m_mergeGapSpin->setValue(
        static_cast<double>(
            defaults.mergeGapMilliseconds
            )
        / 1000.0
        );

    m_elevatedSpin->setRange(
        1,
        1000000
        );

    m_elevatedSpin->setValue(
        defaults.elevatedEventThreshold
        );

    m_errorCriticalSpin->setRange(
        1,
        1000000
        );

    m_errorCriticalSpin->setValue(
        defaults.errorCriticalThreshold
        );

    burstForm->addRow(
        tr("Window:"),
        m_windowSpin
        );

    burstForm->addRow(
        tr("Merge gap:"),
        m_mergeGapSpin
        );

    burstForm->addRow(
        tr("WARN/ERROR/CRITICAL events:"),
        m_elevatedSpin
        );

    burstForm->addRow(
        tr("ERROR/CRITICAL events:"),
        m_errorCriticalSpin
        );

    burstLayout->addLayout(
        burstForm
        );

    layout->addWidget(
        m_burstGroup
        );

    /*
     * ---------------------------------------------------------
     * Validation / actions
     * ---------------------------------------------------------
     */

    m_validationLabel->setWordWrap(
        true
        );

    layout->addWidget(
        m_validationLabel
        );

    connect(
        m_baselineCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            refreshTimeRangeOptions();
            updateValidation();
            applySharedBurstDefaults();
        }
        );

    connect(
        m_comparisonCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            refreshTimeRangeOptions();
            updateValidation();
            applySharedBurstDefaults();
        }
        );

    const auto timeRangeChanged =
        [this](bool) {
            updateValidation();
            applySharedBurstDefaults();
        };

    connect(
        m_baselineTimeRangeCheck,
        &QCheckBox::toggled,
        this,
        timeRangeChanged
        );

    connect(
        m_comparisonTimeRangeCheck,
        &QCheckBox::toggled,
        this,
        timeRangeChanged
        );

    connect(
        m_buttons,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept
        );

    connect(
        m_buttons,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
        );

    layout->addWidget(
        m_buttons
        );

    m_baselineCombo->setObjectName(
        QStringLiteral("baselineSessionCombo")
        );

    m_comparisonCombo->setObjectName(
        QStringLiteral("comparisonSessionCombo")
        );

    m_burstGroup->setObjectName(
        QStringLiteral("burstComparisonGroup")
        );

    m_buttons->setObjectName(
        QStringLiteral("comparisonDialogButtons")
        );

    populateSessions(
        initialBaselineSessionId,
        initialComparisonSessionId
        );

    refreshTimeRangeOptions();
    applySharedBurstDefaults();
    updateValidation();
}

QString InvestigationComparisonDialog::
    baselineSessionId() const
{
    return m_baselineCombo
        ->currentData()
        .toString();
}

QString InvestigationComparisonDialog::
    comparisonSessionId() const
{
    return m_comparisonCombo
        ->currentData()
        .toString();
}

std::optional<BurstDetectionSettings>
    InvestigationComparisonDialog::
    burstSettings() const
{
    if (!m_burstGroup->isChecked()) {
        return std::nullopt;
    }

    BurstDetectionSettings settings;

    settings.windowMilliseconds =
        std::max<qint64>(
            1,
            static_cast<qint64>(
                std::llround(
                    m_windowSpin->value()
                    * 1000.0
                    )
                )
            );

    settings.mergeGapMilliseconds =
        std::max<qint64>(
            0,
            static_cast<qint64>(
                std::llround(
                    m_mergeGapSpin->value()
                    * 1000.0
                    )
                )
            );

    settings.elevatedEventThreshold =
        m_elevatedSpin->value();

    settings.errorCriticalThreshold =
        m_errorCriticalSpin->value();

    return settings;
}

void InvestigationComparisonDialog::
    populateSessions(
        const QString &initialBaselineSessionId,
        const QString &initialComparisonSessionId
        )
{
    m_baselineCombo->clear();
    m_comparisonCombo->clear();

    if (m_workspace == nullptr) {
        return;
    }

    for (
        int index = 0;
        index < m_workspace->sessionCount();
        ++index
        ) {
        const InvestigationSession *session =
            m_workspace->sessionAt(
                index
                );

        if (session == nullptr) {
            continue;
        }

        const QString displayText =
            sessionDisplayText(
                *session
                );

        const QString sessionId =
            session->id();

        m_baselineCombo->addItem(
            displayText,
            sessionId
            );

        m_comparisonCombo->addItem(
            displayText,
            sessionId
            );

        const int baselineItem =
            m_baselineCombo->count()
            - 1;

        const int comparisonItem =
            m_comparisonCombo->count()
            - 1;

        const QString sourcePath =
            session
                ->sourceMetadata()
                .sourcePath;

        if (!sourcePath.isEmpty()) {
            m_baselineCombo->setItemData(
                baselineItem,
                sourcePath,
                Qt::ToolTipRole
                );

            m_comparisonCombo->setItemData(
                comparisonItem,
                sourcePath,
                Qt::ToolTipRole
                );
        }
    }

    QString comparisonId =
        initialComparisonSessionId;

    if (m_workspace->indexOfSession(
            comparisonId
            )
        < 0) {
        const InvestigationSession *activeSession =
            m_workspace->activeSession();

        comparisonId =
            activeSession != nullptr
                ? activeSession->id()
                : QString();
    }

    const int comparisonIndex =
        m_comparisonCombo->findData(
            comparisonId
            );

    if (comparisonIndex >= 0) {
        m_comparisonCombo->setCurrentIndex(
            comparisonIndex
            );
    }

    QString baselineId =
        initialBaselineSessionId;

    if (m_workspace->indexOfSession(
            baselineId
            )
            < 0
        || baselineId
               == comparisonId) {
        baselineId.clear();

        for (
            int index = 0;
            index < m_baselineCombo->count();
            ++index
            ) {
            const QString candidateId =
                m_baselineCombo
                    ->itemData(index)
                    .toString();

            if (candidateId
                == comparisonId) {
                continue;
            }

            baselineId =
                candidateId;

            break;
        }
    }

    const int baselineIndex =
        m_baselineCombo->findData(
            baselineId
            );

    if (baselineIndex >= 0) {
        m_baselineCombo->setCurrentIndex(
            baselineIndex
            );
    }
}

void InvestigationComparisonDialog::
    applySharedBurstDefaults()
{
    if (m_workspace == nullptr) {
        return;
    }

    const int baselineIndex =
        m_workspace->indexOfSession(
            baselineSessionId()
            );

    const int comparisonIndex =
        m_workspace->indexOfSession(
            comparisonSessionId()
            );

    if (baselineIndex < 0
        || comparisonIndex < 0
        || baselineIndex
               == comparisonIndex) {
        return;
    }

    const InvestigationSession *baselineSession =
        m_workspace->sessionAt(
            baselineIndex
            );

    const InvestigationSession *comparisonSession =
        m_workspace->sessionAt(
            comparisonIndex
            );

    if (baselineSession == nullptr
        || comparisonSession == nullptr) {
        return;
    }

    const auto baselineRange =
        baselineTimeRange();

    const auto comparisonRange =
        comparisonTimeRange();

    QVector<InvestigationRecord> scopedBaseline;
    QVector<InvestigationRecord> scopedComparison;

    const auto &allBaseline =
        baselineSession->investigationController()
            ->allRecords();

    const auto &allComparison =
        comparisonSession->investigationController()
            ->allRecords();

    if (baselineRange.has_value()) {
        scopedBaseline =
            selectComparisonTimeRangeRecords(
                allBaseline,
                *baselineRange
                );
    }

    if (comparisonRange.has_value()) {
        scopedComparison =
            selectComparisonTimeRangeRecords(
                allComparison,
                *comparisonRange
                );
    }

    const auto &baselineRecords =
        baselineRange.has_value()
            ? scopedBaseline
            : allBaseline;

    const auto &comparisonRecords =
        comparisonRange.has_value()
            ? scopedComparison
            : allComparison;

    InvestigationCadenceAnalyzer analyzer;

    InvestigationCadence baselineCadence =
        analyzer.analyze(baselineRecords);

    InvestigationCadence comparisonCadence =
        analyzer.analyze(comparisonRecords);

    /*
     * If a selected time range is too sparse for
     * adaptive recommendations, use the corresponding
     * complete session's cadence instead.
     *
     * This affects automatic shared settings only.
     * The actual comparison still uses the selected
     * record populations.
     */
    if (baselineRange.has_value()
        && baselineCadence.usesFallbackRecommendation) {
        baselineCadence =
            analyzer.analyze(allBaseline);
    }

    if (comparisonRange.has_value()
        && comparisonCadence.usesFallbackRecommendation) {
        comparisonCadence =
            analyzer.analyze(allComparison);
    }

    /*
     * Use one shared setting for both sides.
     *
     * Choosing the larger recommendation avoids
     * using a window tuned for a faster session
     * that may be too narrow to detect meaningful
     * episodes in the slower session.
     */
    const qint64 sharedWindow =
        std::max(
            baselineCadence
                .recommendedBurstWindowMilliseconds,
            comparisonCadence
                .recommendedBurstWindowMilliseconds
            );

    const qint64 sharedMergeGap =
        std::max(
            baselineCadence
                .recommendedMergeGapMilliseconds,
            comparisonCadence
                .recommendedMergeGapMilliseconds
            );

    m_windowSpin->setValue(
        static_cast<double>(
            sharedWindow
            )
        / 1000.0
        );

    m_mergeGapSpin->setValue(
        static_cast<double>(
            sharedMergeGap
            )
        / 1000.0
        );
}

void InvestigationComparisonDialog::
    updateValidation()
{
    QPushButton *okButton =
        m_buttons->button(
            QDialogButtonBox::Ok
            );

    const QString baselineId =
        baselineSessionId();

    const QString comparisonId =
        comparisonSessionId();

    const bool valid =
        !baselineId.isEmpty()
        && !comparisonId.isEmpty()
        && baselineId
               != comparisonId;

    okButton->setEnabled(
        valid
        );

    if (valid) {
        m_validationLabel->clear();
        m_validationLabel->setVisible(
            false
            );

        return;
    }

    m_validationLabel->setText(
        tr(
            "Baseline and Comparison must be "
            "different open sessions."
            )
        );

    m_validationLabel->setVisible(
        true
        );
}

const InvestigationSession *
InvestigationComparisonDialog::selectedSession(
    const QString &sessionId
    ) const
{
    if (m_workspace == nullptr) {
        return nullptr;
    }

    const int index =
        m_workspace->indexOfSession(sessionId);

    return index >= 0
               ? m_workspace->sessionAt(index)
               : nullptr;
}

std::optional<InvestigationComparisonTimeRange>
InvestigationComparisonDialog::baselineTimeRange() const
{
    return m_baselineTimeRangeCheck->isChecked()
    ? activeRangeFor(
          selectedSession(baselineSessionId())
          )
    : std::nullopt;
}

std::optional<InvestigationComparisonTimeRange>
InvestigationComparisonDialog::comparisonTimeRange() const
{
    return m_comparisonTimeRangeCheck->isChecked()
    ? activeRangeFor(
          selectedSession(comparisonSessionId())
          )
    : std::nullopt;
}

void InvestigationComparisonDialog::
    refreshTimeRangeOptions()
{
    const auto configure =
        [this](
            QCheckBox *checkbox,
            const QString &sessionId
            ) {
            const InvestigationSession *session =
                selectedSession(sessionId);

            const auto range =
                activeRangeFor(session);

            const bool available =
                session != nullptr
                && range.has_value()
                && containsRecordsInRange(
                    *session,
                    *range
                    );

            checkbox->setEnabled(available);

            if (!available) {
                checkbox->setChecked(false);
            }

            checkbox->setToolTip(
                available
                    ? tr(
                          "Compare records within this "
                          "session's active time boundaries. "
                          "All other filters are ignored."
                          )
                    : tr(
                          "This session has no usable active "
                          "time range containing records."
                          )
                );
        };

    configure(
        m_baselineTimeRangeCheck,
        baselineSessionId()
        );

    configure(
        m_comparisonTimeRangeCheck,
        comparisonSessionId()
        );
}