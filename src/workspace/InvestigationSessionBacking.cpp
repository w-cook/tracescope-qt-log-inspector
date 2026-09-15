#include "InvestigationSessionBacking.h"

#include <utility>

InvestigationSessionBacking
InvestigationSessionBacking::sourceBacked(
    InvestigationExternalSourceBinding
        externalSource
    )
{
    InvestigationSessionBacking backing;

    backing.m_externalSource =
        std::move(externalSource);

    return backing;
}

InvestigationSessionBacking
InvestigationSessionBacking::snapshotBacked(
    InvestigationSnapshotBinding snapshot
    )
{
    InvestigationSessionBacking backing;

    backing.m_snapshot =
        std::move(snapshot);

    return backing;
}

InvestigationSessionBacking
InvestigationSessionBacking::hybrid(
    InvestigationExternalSourceBinding
        externalSource,
    InvestigationSnapshotBinding snapshot
    )
{
    InvestigationSessionBacking backing;

    backing.m_externalSource =
        std::move(externalSource);

    backing.m_snapshot =
        std::move(snapshot);

    return backing;
}

InvestigationSessionBackingMode
InvestigationSessionBacking::mode() const
{
    if (m_externalSource.has_value()
        && m_snapshot.has_value()) {
        return InvestigationSessionBackingMode::
            Hybrid;
    }

    if (m_snapshot.has_value()) {
        return InvestigationSessionBackingMode::
            SnapshotBacked;
    }

    /*
     * The public construction/transitions guarantee
     * that at least one backing always exists.
     */
    return InvestigationSessionBackingMode::
        SourceBacked;
}

bool InvestigationSessionBacking::
    hasExternalSource() const
{
    return m_externalSource.has_value();
}

bool InvestigationSessionBacking::
    hasSnapshot() const
{
    return m_snapshot.has_value();
}

const InvestigationExternalSourceBinding *
    InvestigationSessionBacking::
    externalSource() const
{
    if (!m_externalSource.has_value()) {
        return nullptr;
    }

    return &*m_externalSource;
}

InvestigationExternalSourceBinding *
InvestigationSessionBacking::externalSource()
{
    if (!m_externalSource.has_value()) {
        return nullptr;
    }

    return &*m_externalSource;
}

const InvestigationSnapshotBinding *
InvestigationSessionBacking::snapshot() const
{
    if (!m_snapshot.has_value()) {
        return nullptr;
    }

    return &*m_snapshot;
}

InvestigationSnapshotBinding *
InvestigationSessionBacking::snapshot()
{
    if (!m_snapshot.has_value()) {
        return nullptr;
    }

    return &*m_snapshot;
}

void InvestigationSessionBacking::
    attachSnapshot(
        InvestigationSnapshotBinding snapshot
        )
{
    m_snapshot =
        std::move(snapshot);
}

bool InvestigationSessionBacking::
    detachSnapshot()
{
    /*
     * Never allow the value object to enter an
     * unbacked state.
     */
    if (!m_externalSource.has_value()) {
        return false;
    }

    m_snapshot.reset();

    return true;
}

void InvestigationSessionBacking::
    connectExternalSource(
        InvestigationExternalSourceBinding
            externalSource
        )
{
    m_externalSource =
        std::move(externalSource);
}

bool InvestigationSessionBacking::
    disconnectExternalSource()
{
    /*
     * Never allow the value object to enter an
     * unbacked state.
     */
    if (!m_snapshot.has_value()) {
        return false;
    }

    m_externalSource.reset();

    return true;
}