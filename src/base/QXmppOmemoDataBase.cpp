// SPDX-FileCopyrightText: 2021 Germán Márquez Mejía <mancho@olomono.de>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppConstants_p.h"
#include "QXmppOmemoElement_p.h"
#include "QXmppOmemoEnvelope_p.h"
#include "QXmppUtils.h"

#include <QDomElement>
#include <QXmlStreamWriter>

/// \cond
///
/// \class QXmppOmemoEnvelope
///
/// \brief The QXmppOmemoEnvelope class represents an OMEMO envelope as
/// defined by \xep{0384, OMEMO Encryption}.
///

///
/// Returns the ID of the recipient's device.
///
/// The ID is 0 if it is unset.
///
/// \return the recipient's device ID
///
uint32_t QXmppOmemoEnvelope::recipientDeviceId() const
{
    return m_recipientDeviceId;
}

///
/// Sets the ID of the recipient's device.
///
/// The ID must be at least 1 and at most \c std::numeric_limits<int32_t>::max().
///
/// \param id recipient's device ID
///
void QXmppOmemoEnvelope::setRecipientDeviceId(uint32_t id)
{
    m_recipientDeviceId = id;
}

///
/// Returns true if a pre-key was used to prepare this envelope.
///
/// The default is false.
///
/// \return true if a pre-key was used to prepare this envelope, otherwise false
///
bool QXmppOmemoEnvelope::isUsedForKeyExchange() const
{
    return m_isUsedForKeyExchange;
}

///
/// Sets whether a pre-key was used to prepare this envelope.
///
/// \param isUsed whether a pre-key was used to prepare this envelope
///
void QXmppOmemoEnvelope::setIsUsedForKeyExchange(bool isUsed)
{
    m_isUsedForKeyExchange = isUsed;
}

///
/// Returns the BLOB containing the data for the underlying double ratchet library.
///
/// It should be treated like an obscure BLOB being passed as is to the ratchet
/// library for further processing.
///
/// \return the binary data for the ratchet library
///
QByteArray QXmppOmemoEnvelope::data() const
{
    return m_data;
}

///
/// Sets the BLOB containing the data from the underlying double ratchet library.
///
/// It should be treated like an obscure BLOB produced by the ratchet library.
///
/// \param data binary data from the ratchet library
///
void QXmppOmemoEnvelope::setData(const QByteArray &data)
{
    m_data = data;
}

void QXmppOmemoEnvelope::parse(const QDomElement &element)
{
    m_recipientDeviceId = element.attribute(QStringLiteral("rid")).toInt();

    const auto isUsedForKeyExchange = element.attribute(QStringLiteral("kex"));
    if (isUsedForKeyExchange == QStringLiteral("true") ||
        isUsedForKeyExchange == QStringLiteral("1")) {
        m_isUsedForKeyExchange = true;
    }

    m_data = QByteArray::fromBase64(element.text().toLatin1());
}

void QXmppOmemoEnvelope::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("key"));
    writer->writeAttribute(QStringLiteral("rid"), QString::number(m_recipientDeviceId));

    if (m_isUsedForKeyExchange) {
        helperToXmlAddAttribute(writer, QStringLiteral("kex"), QStringLiteral("true"));
    }

    writer->writeCharacters(m_data.toBase64());
    writer->writeEndElement();
}

///
/// Determines whether the given DOM element is an OMEMO envelope.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO envelope, otherwise false
///
bool QXmppOmemoEnvelope::isOmemoEnvelope(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("key") &&
        element.namespaceURI() == ns_omemo_2;
}

///
/// \class QXmppOmemoElement
///
/// \brief The QXmppOmemoElement class represents an OMEMO element as
/// defined by \xep{0384, OMEMO Encryption}.
///

///
/// Returns the ID of the sender's device.
///
/// The ID is 0 if it is unset.
///
/// \return the sender's device ID
///
uint32_t QXmppOmemoElement::senderDeviceId() const
{
    return m_senderDeviceId;
}

///
/// Sets the ID of the sender's device.
///
/// The ID must be at least 1 and at most
/// \c std::numeric_limits<int32_t>::max().
///
/// \param id sender's device ID
///
void QXmppOmemoElement::setSenderDeviceId(uint32_t id)
{
    m_senderDeviceId = id;
}

///
/// Returns the payload which consists of the encrypted SCE envelope.
///
/// \return the encrypted payload
///
QByteArray QXmppOmemoElement::payload() const
{
    return m_payload;
}

///
/// Sets the payload which consists of the encrypted SCE envelope.
///
/// \param payload encrypted payload
///
void QXmppOmemoElement::setPayload(const QByteArray &payload)
{
    m_payload = payload;
}

///
/// Searches for an OMEMO envelope by its recipient JID and device ID.
///
/// \param recipientJid bare JID of the recipient
/// \param recipientDeviceId ID of the recipient's device
///
/// \return the found OMEMO envelope
///
std::optional<QXmppOmemoEnvelope> QXmppOmemoElement::searchEnvelope(const QString &recipientJid, uint32_t recipientDeviceId) const
{
    for (auto itr = m_envelopes.constFind(recipientJid);
         itr != m_envelopes.constEnd() && itr.key() == recipientJid;
         ++itr) {
        const auto &envelope = itr.value();
        if (envelope.recipientDeviceId() == recipientDeviceId) {
            return envelope;
        }
    }

    return std::nullopt;
}

///
/// Adds an OMEMO envelope.
///
/// If a full JID is passed as recipientJid, it is converted into a bare JID.
///
/// \see QXmppOmemoEnvelope
///
/// \param recipientJid bare JID of the recipient
/// \param envelope OMEMO envelope
///
void QXmppOmemoElement::addEnvelope(const QString &recipientJid, const QXmppOmemoEnvelope &envelope)
{
    m_envelopes.insert(QXmppUtils::jidToBareJid(recipientJid), envelope);
}

void QXmppOmemoElement::parse(const QDomElement &element)
{
    const auto header = element.firstChildElement(QStringLiteral("header"));

    m_senderDeviceId = header.attribute(QStringLiteral("sid")).toInt();

    for (auto recipient = header.firstChildElement(QStringLiteral("keys"));
         !recipient.isNull();
         recipient = recipient.nextSiblingElement(QStringLiteral("keys"))) {
        const auto recipientJid = recipient.attribute(QStringLiteral("jid"));

        for (auto envelope = recipient.firstChildElement(QStringLiteral("key"));
             !envelope.isNull();
             envelope = envelope.nextSiblingElement(QStringLiteral("key"))) {
            QXmppOmemoEnvelope omemoEnvelope;
            omemoEnvelope.parse(envelope);
            addEnvelope(recipientJid, omemoEnvelope);
        }
    }

    m_payload = QByteArray::fromBase64(element.firstChildElement(QStringLiteral("payload")).text().toLatin1());
}

void QXmppOmemoElement::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("encrypted"));
    writer->writeDefaultNamespace(ns_omemo_2);

    writer->writeStartElement(QStringLiteral("header"));
    writer->writeAttribute(QStringLiteral("sid"), QString::number(m_senderDeviceId));

    const auto recipientJids = m_envelopes.uniqueKeys();
    for (const auto &recipientJid : recipientJids) {
        writer->writeStartElement(QStringLiteral("keys"));
        writer->writeAttribute(QStringLiteral("jid"), recipientJid);

        for (auto itr = m_envelopes.constFind(recipientJid);
             itr != m_envelopes.constEnd() && itr.key() == recipientJid;
             ++itr) {
            const auto &envelope = itr.value();
            envelope.toXml(writer);
        }

        writer->writeEndElement();  // keys
    }

    writer->writeEndElement();  // header

    // The payload element is only included if there is a payload.
    // An empty OMEMO message does not contain a payload.
    if (!m_payload.isEmpty()) {
        writer->writeTextElement(QStringLiteral("payload"), m_payload.toBase64());
    }

    writer->writeEndElement();  // encrypted
}

///
/// Determines whether the given DOM element is an OMEMO element.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO element, otherwise false
///
bool QXmppOmemoElement::isOmemoElement(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("encrypted") &&
        element.namespaceURI() == ns_omemo_2;
}
/// \endcond
