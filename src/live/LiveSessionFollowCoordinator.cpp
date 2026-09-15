#include "LiveSessionFollowCoordinator.h"

#include <utility>

#include "../workspace/InvestigationSession.h"

LiveSessionFollowCoordinator::
    LiveSessionFollowCoordinator(
        InvestigationSession &session,
        QObject *parent
        )
    : QObject(parent),
    m_session(&session),
    m_follower(
        session
            .sourceMetadata()
            .sourcePath,
        this
        ),
    m_lineImportAdapter(
        session.importProfile()
        ),
    m_structuredJsonSourceContext(
        session
            .importProfile()
            .recordPath
        ),
    m_structuredJsonImportAdapter(
        session.importProfile()
        ),
    m_structuredXmlSourceContext(
        session
            .importProfile()
            .recordPath
        ),
    m_structuredXmlImportAdapter(
        session.importProfile()
        ),
    m_ingestionKind(
        ingestionKindForProfile(
            session.importProfile()
            )
        )
{
    m_pollTimer.setInterval(
        DefaultPollIntervalMilliseconds
        );

    connect(
        &m_pollTimer,
        &QTimer::timeout,
        this,
        &LiveSessionFollowCoordinator::
        pollFromTimer
        );

    connect(
        &m_follower,
        &LiveFileFollower::stateChanged,
        this,
        &LiveSessionFollowCoordinator::
        stateChanged
        );
}

bool LiveSessionFollowCoordinator::
    supportsProfile(
        const ImportProfile &profile
        )
{
    return ingestionKindForProfile(
               profile
               )
           != IngestionKind::Unsupported;
}

bool LiveSessionFollowCoordinator::
    isSupported() const
{
    return m_ingestionKind
           != IngestionKind::Unsupported;
}

const LiveFileFollowState &
LiveSessionFollowCoordinator::state() const
{
    return m_follower.state();
}

LiveSessionFollowStartResult
LiveSessionFollowCoordinator::start()
{
    LiveSessionFollowStartResult result;

    if (!isSupported()) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The session's importer does not "
                "currently support live following."
                );

        return result;
    }

    /*
     * Once a live context has been initialized
     * successfully, Stop does not discard it.
     *
     * On a same-generation restart that context is
     * already the exact parser/framing state that
     * corresponds to the follower's retained byte
     * offset.
     *
     * If the source changed generation while stopped,
     * that retained context must instead be reset
     * before any bytes from the new generation are
     * interpreted.
     */
    const bool isRestart =
        m_hasInitializedLiveContext;

    const quint64 previousSourceGeneration =
        m_follower
            .state()
            .sourceGeneration();

    if (!m_follower.start()) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live source could not be "
                "started for following."
                );

        return result;
    }

    result.baselineByteCount =
        m_follower.state().readOffset();

    const quint64 sourceGeneration =
        m_follower
            .state()
            .sourceGeneration();

    result.sourceGenerationChanged =
        sourceGeneration
        != previousSourceGeneration;

    /*
     * Restart after a previously successful live
     * session.
     *
     * Same generation:
     * Preserve all existing incremental state.
     *
     * New generation:
     * Discard framing/parser state belonging to the
     * previous physical source and begin fresh at
     * byte zero.
     */
    if (isRestart) {
        if (result.sourceGenerationChanged) {
            switch (m_ingestionKind) {
            case IngestionKind::Line:
                m_lineSourceContext
                    .beginSourceGeneration(
                        sourceGeneration
                        );

                m_lineImportAdapter
                    .beginSourceGeneration();
                break;

            case IngestionKind::StructuredJson:
                m_structuredJsonSourceContext
                    .beginSourceGeneration(
                        sourceGeneration
                        );
                break;

            case IngestionKind::StructuredXml:
                m_structuredXmlSourceContext
                    .beginSourceGeneration(
                        sourceGeneration
                        );
                break;

            case IngestionKind::Unsupported:
                break;
            }
        }

        m_pollTimer.start();

        if (result.sourceGenerationChanged) {
            emit sourceGenerationChanged(
                sourceGeneration
                );
        }

        return result;
    }

    /*
     * First Start after initial import/reload.
     *
     * The follower captured current EOF as the live
     * boundary. Reconstruct format-specific state
     * from exactly that existing physical range
     * without importing those records again.
     */
    switch (m_ingestionKind) {
    case IngestionKind::Line: {
        const LiveLineSourceInitializationResult
            sourceInitialization =
            m_lineSourceContext
                .initializeFromExistingFile(
                    m_follower.sourcePath(),
                    result.baselineByteCount,
                    sourceGeneration
                    );

        if (!sourceInitialization.succeeded) {
            m_follower.stop();

            result.succeeded = false;

            result.errorMessage =
                sourceInitialization.errorMessage;

            return result;
        }

        /*
         * Static line importers may already have
         * treated an unterminated final physical
         * line as a record. Do not ambiguously join
         * later bytes to that already-imported
         * evidence.
         *
         * This restriction belongs only to initial
         * live-follow setup. A later Stop -> Start
         * preserves the live framer itself instead.
         */
        if (result.baselineByteCount > 0
            && !sourceInitialization
                    .endsAtLineBoundary) {
            m_follower.stop();

            result.succeeded = false;

            result.errorMessage =
                QStringLiteral(
                    "Live following cannot begin "
                    "because the existing source "
                    "does not end at a physical "
                    "line boundary."
                    );

            return result;
        }

        const IncrementalImportInitializationResult
            importInitialization =
            m_lineImportAdapter
                .initializeFromExistingFile(
                    m_follower.sourcePath(),
                    result.baselineByteCount
                    );

        if (!importInitialization.succeeded) {
            m_follower.stop();

            result.succeeded = false;

            result.errorMessage =
                importInitialization.errorMessage;

            return result;
        }

        break;
    }

    case IngestionKind::StructuredJson: {
        const
            LiveStructuredJsonSourceInitializationResult
                sourceInitialization =
            m_structuredJsonSourceContext
                .initializeFromExistingFile(
                    m_follower.sourcePath(),
                    result.baselineByteCount,
                    sourceGeneration
                    );

        if (!sourceInitialization.succeeded) {
            m_follower.stop();

            result.succeeded = false;

            result.errorMessage =
                sourceInitialization.errorMessage;

            return result;
        }

        break;
    }

    case IngestionKind::StructuredXml: {
        const
            LiveStructuredXmlSourceInitializationResult
                sourceInitialization =
            m_structuredXmlSourceContext
                .initializeFromExistingFile(
                    m_follower.sourcePath(),
                    result.baselineByteCount,
                    sourceGeneration
                    );

        if (!sourceInitialization.succeeded) {
            m_follower.stop();

            result.succeeded = false;

            result.errorMessage =
                sourceInitialization.errorMessage;

            return result;
        }

        break;
    }

    case IngestionKind::Unsupported:
        m_follower.stop();

        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The session's importer does not "
                "currently support live following."
                );

        return result;
    }

    m_hasInitializedLiveContext = true;

    m_pollTimer.start();

    return result;
}

bool LiveSessionFollowCoordinator::pause()
{
    return m_follower.pause();
}

bool LiveSessionFollowCoordinator::resume()
{
    return m_follower.resume();
}

bool LiveSessionFollowCoordinator::stop()
{
    m_pollTimer.stop();

    return m_follower.stop();
}

LiveSessionFollowPollResult
LiveSessionFollowCoordinator::pollOnce(
    qint64 maxByteCount
    )
{
    LiveSessionFollowPollResult result;

    LiveFileReadResult readResult =
        m_follower.readAvailableBytes(
            maxByteCount
            );

    result.observation =
        readResult.observation;

    /*
     * Source replacement/truncation invalidates the
     * active framing state before any bytes from the
     * new generation are interpreted.
     */
    if (readResult.observation.kind
        == LiveFileObservationKind::
        SourceReset) {
        result.sourceGenerationChanged =
            true;

        const quint64 sourceGeneration =
            m_follower
                .state()
                .sourceGeneration();

        switch (m_ingestionKind) {
        case IngestionKind::Line:
            m_lineSourceContext
                .beginSourceGeneration(
                    sourceGeneration
                    );

            m_lineImportAdapter
                .beginSourceGeneration();
            break;

        case IngestionKind::StructuredJson:
            m_structuredJsonSourceContext
                .beginSourceGeneration(
                    sourceGeneration
                    );
            break;

        case IngestionKind::StructuredXml:
            m_structuredXmlSourceContext
                .beginSourceGeneration(
                    sourceGeneration
                    );
            break;

        case IngestionKind::Unsupported:
            break;
        }
    }

    if (!readResult.succeeded) {
        result.succeeded = false;
        result.errorMessage =
            readResult.errorMessage;

        return result;
    }

    /*
     * No bytes may mean no growth, pause, missing
     * source, or an empty new generation.
     */
    if (readResult.bytes.isEmpty()) {
        return result;
    }

    ImportResult importResult;

    switch (m_ingestionKind) {
    case IngestionKind::Line: {
        LiveFramedLineBatch batch =
            m_lineSourceContext.appendBytes(
                std::move(
                    readResult.bytes
                    )
                );

        /*
         * An incomplete trailing physical line stays
         * inside the framer until a later poll.
         */
        if (batch.lines.isEmpty()) {
            return result;
        }

        importResult =
            m_lineImportAdapter.importBatch(
                batch,
                m_follower.sourcePath()
                );

        break;
    }

    case IngestionKind::StructuredJson: {
        LiveStructuredJsonSourceAppendResult
            appendResult =
            m_structuredJsonSourceContext
                .appendBytes(
                    std::move(
                        readResult.bytes
                        )
                    );

        if (!appendResult.succeeded) {
            result.succeeded = false;

            result.errorMessage =
                appendResult.errorMessage;

            return result;
        }

        /*
         * An incomplete JSON object stays inside the
         * structured framer until a later poll.
         */
        if (appendResult
                .batch
                .records
                .isEmpty()) {
            return result;
        }

        importResult =
            m_structuredJsonImportAdapter
                .importBatch(
                    appendResult.batch,
                    m_follower.sourcePath()
                    );

        break;
    }

    case IngestionKind::StructuredXml: {
        LiveStructuredXmlSourceAppendResult
            appendResult =
            m_structuredXmlSourceContext
                .appendBytes(
                    std::move(
                        readResult.bytes
                        )
                    );

        if (!appendResult.succeeded) {
            result.succeeded = false;

            result.errorMessage =
                appendResult.errorMessage;

            return result;
        }

        /*
         * An incomplete XML record remains inside the
         * persistent XML parser until a later poll
         * provides its closing bytes.
         */
        if (appendResult
                .batch
                .records
                .isEmpty()) {
            return result;
        }

        importResult =
            m_structuredXmlImportAdapter
                .importBatch(
                    appendResult.batch,
                    m_follower.sourcePath()
                    );

        break;
    }

    case IngestionKind::Unsupported:
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The active live-follow importer "
                "is unsupported."
                );

        return result;
    }

    result.processedRecordCount =
        importResult.processedRecordCount;

    result.importedRecordCount =
        importResult.importedRecordCount();

    /*
     * This is the single convergence point for every
     * supported live-ingestion strategy.
     *
     * Do not rebuild the session or reapply filters.
     * InvestigationController::appendRecords() lets
     * the existing proxy evaluate inserted rows
     * against its already-active filter state.
     */
    m_session->appendLiveImportResult(
        std::move(
            importResult
            )
        );

    return result;
}

int LiveSessionFollowCoordinator::
    pollIntervalMilliseconds() const
{
    return m_pollTimer.interval();
}

bool LiveSessionFollowCoordinator::
    setPollIntervalMilliseconds(
        int intervalMilliseconds
        )
{
    if (intervalMilliseconds <= 0) {
        return false;
    }

    m_pollTimer.setInterval(
        intervalMilliseconds
        );

    return true;
}

LiveSessionFollowCoordinator::IngestionKind
    LiveSessionFollowCoordinator::
    ingestionKindForProfile(
        const ImportProfile &profile
        )
{
    if (LiveLineImportAdapter(
            profile
            ).isSupported()) {
        return IngestionKind::Line;
    }

    if (LiveStructuredJsonImportAdapter(
            profile
            ).isSupported()) {
        return IngestionKind::
            StructuredJson;
    }

    if (LiveStructuredXmlImportAdapter(
            profile
            ).isSupported()) {
        return IngestionKind::
            StructuredXml;
    }

    return IngestionKind::Unsupported;
}

void LiveSessionFollowCoordinator::
    pollFromTimer()
{
    const LiveSessionFollowPollResult result =
        pollOnce();

    if (!result.succeeded) {
        emit pollError(
            result.errorMessage
            );

        return;
    }

    if (result.sourceGenerationChanged) {
        emit sourceGenerationChanged(
            m_follower
                .state()
                .sourceGeneration()
            );
    }

    if (result.processedRecordCount > 0) {
        emit sessionUpdated();
    }
}