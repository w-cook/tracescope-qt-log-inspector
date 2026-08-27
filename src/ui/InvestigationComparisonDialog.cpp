#include "InvestigationComparisonDialog.h"

#include <algorithm>
#include <cmath>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include "../analysis/InvestigationCadenceAnalyzer.h"
#include "../workspace/InvestigationSession.h"
#include "../workspace/InvestigationWorkspace.h"

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
    m_comparisonCombo(
        new QComboBox(this)
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
        520
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        8
        );

    auto *description =
        new QLabel(
            tr(
                "Compare two complete imported sessions. "
                "The baseline is evaluated against the "
                "comparison session, and every delta is "
                "calculated as Comparison − Baseline."
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
        tr("Comparison:"),
        m_comparisonCombo
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
                "Window and merge-gap values default to "
                "shared automatic recommendations derived "
                "from the complete cadence of both selected "
                "sessions. The same explicit settings are "
                "then used for both sides of the comparison. "
                "You may adjust them before creating the "
                "comparison."
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
            updateValidation();
            applySharedBurstDefaults();
        }
        );

    connect(
        m_comparisonCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            updateValidation();
            applySharedBurstDefaults();
        }
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

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence baselineCadence =
        analyzer.analyze(
            baselineSession
                ->investigationController()
                ->allRecords()
            );

    const InvestigationCadence comparisonCadence =
        analyzer.analyze(
            comparisonSession
                ->investigationController()
                ->allRecords()
            );

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