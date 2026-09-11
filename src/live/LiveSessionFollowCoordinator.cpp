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
    m_importAdapter(
        session.importProfile()
        )
{
}

bool LiveSessionFollowCoordinator::
    supportsProfile(
        const ImportProfile &profile
        )
{
    return LiveLineImportAdapter(
               profile
               ).isSupported();
}

bool LiveSessionFollowCoordinator::
    isSupported() const
{
    return m_importAdapter.isSupported();
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

    if (!m_importAdapter.isSupported()) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The session's importer does not "
                "currently support line-oriented "
                "live following."
                );

        return result;
    }

    /*
     * Capture the physical source boundary first.
     * Everything before this byte position belongs
     * to the already-imported static session.
     */
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

    /*
     * Seed physical line numbering from exactly the
     * same byte range captured by the follower.
     */
    const LiveLineSourceInitializationResult
        sourceInitialization =
        m_sourceContext
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
     * Static line importers may already have treated
     * an unterminated final physical line as a record.
     * Continuing that physical line live would make
     * evidence identity ambiguous, so activation is
     * rejected conservatively.
     */
    if (result.baselineByteCount > 0
        && !sourceInitialization
                .endsAtLineBoundary) {
        m_follower.stop();

        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "Live following cannot begin because "
                "the existing source does not end at "
                "a physical line boundary."
                );

        return result;
    }

    /*
     * Stateful line importers such as CSV/TSV and
     * IIS reconstruct only the parser state that
     * existed inside the same captured baseline.
     */
    const IncrementalImportInitializationResult
        importInitialization =
        m_importAdapter
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
     * A new physical source generation invalidates
     * framing and format-specific parser state even
     * though previously imported investigation
     * evidence remains intact.
     *
     * Do this even if the subsequent physical read
     * fails; the follower itself has already changed
     * generations at this point.
     */
    if (readResult.observation.kind
        == LiveFileObservationKind::
        SourceReset) {
        result.sourceGenerationChanged = true;

        const quint64 sourceGeneration =
            m_follower
                .state()
                .sourceGeneration();

        m_sourceContext
            .beginSourceGeneration(
                sourceGeneration
                );

        m_importAdapter
            .beginSourceGeneration();
    }

    if (!readResult.succeeded) {
        result.succeeded = false;
        result.errorMessage =
            readResult.errorMessage;

        return result;
    }

    /*
     * No physical bytes may simply mean no growth,
     * a paused follower, a missing source, or an
     * empty new source generation. None requires
     * semantic import work.
     */
    if (readResult.bytes.isEmpty()) {
        return result;
    }

    LiveFramedLineBatch batch =
        m_sourceContext.appendBytes(
            std::move(
                readResult.bytes
                )
            );

    /*
     * An incomplete trailing physical line remains
     * owned by the framer until a later read
     * completes it.
     */
    if (batch.lines.isEmpty()) {
        return result;
    }

    ImportResult importResult =
        m_importAdapter.importBatch(
            batch,
            m_follower.sourcePath()
            );

    result.processedRecordCount =
        importResult.processedRecordCount;

    result.importedRecordCount =
        importResult.importedRecordCount();

    /*
     * Append even when no records were produced:
     * malformed live source records may still carry
     * diagnostics and processed-record accounting
     * that belong to the investigation.
     */
    m_session->appendLiveImportResult(
        std::move(
            importResult
            )
        );

    return result;
}