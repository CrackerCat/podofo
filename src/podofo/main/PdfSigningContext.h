/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_SIGNING_CONTEXT_H
#define PDF_SIGNING_CONTEXT_H

#include "PdfSigner.h"

namespace PoDoFo
{
    class PODOFO_API PdfSignerId final
    {
    public:
        PdfSignerId();
        PdfSignerId(const PdfReference& ref, unsigned signerIndex);

        const PdfReference& GetSignatureRef() const { return m_SignatureRef; }
        unsigned GetSignerIndex() const { return m_SignerIndex; }

        bool operator==(const PdfSignerId& rhs) const
        {
            return m_SignatureRef == rhs.m_SignatureRef && m_SignerIndex == rhs.m_SignerIndex;
        }

        bool operator!=(const PdfSignerId& rhs) const
        {
            return m_SignatureRef != rhs.m_SignatureRef || m_SignerIndex != rhs.m_SignerIndex;
        }

    private:
        PdfReference m_SignatureRef;
        unsigned m_SignerIndex;
    };
}

namespace std
{
    /** Overload hasher for PdfSignerId
     */
    template<>
    struct hash<PoDoFo::PdfSignerId>
    {
        std::size_t operator()(const PoDoFo::PdfSignerId& id) const noexcept
        {
            return id.GetSignatureRef().ObjectNumber() ^ (id.GetSignatureRef().GenerationNumber() << 16)
                ^ ((size_t)id.GetSignerIndex() << 24);
        }
    };
}

namespace PoDoFo
{
    /**
     * Results produced by starting a sequential signing operation
     */
    struct PODOFO_API PdfSigningResults final
    {
        std::unordered_map<PdfSignerId, charbuff> Intermediate;
    };

    /**
     * A context that can be used to customize the signing process.
     * It also enables the sequential, that is a process the intermediate
     * results of singning (normally a hash to sign) that doesn't
     * require a streamline event based processing. It can be issued by starting
     * the process with StartSigning() and finishing it with FinishSigning()
     */
    class PODOFO_API PdfSigningContext final
    {
        friend PODOFO_API void SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
            PdfSignature& signature, PdfSaveOptions saveOptions);
    public:
        PdfSigningContext();
        PdfSignerId AddSigner(const PdfSignature& signature, const std::shared_ptr<PdfSigner>& signer);
        /** Start a sequential signing procedure
         * \return the intermediate results
         */
        PdfSigningResults StartSigning(PdfMemDocument& doc, const std::shared_ptr<StreamDevice>& device);

        /** Finish a sequential signing procedure
         * \param processedResults 
         */
        void FinishSigning(const std::unordered_map<PdfSignerId, charbuff>& processedResults);
        /** Start a event driven signing procedure
         */
        void Sign(PdfMemDocument& doc, StreamDevice& device);

        void SetSaveOptions(PdfSaveOptions options) { m_SaveOptions = options; }
        PdfSaveOptions GetSaveOptions() const { return m_SaveOptions; }

    private:
        struct SignatureAttrs
        {
            unsigned PageIndex = 0u - 1u; // Necessary to correctly recover the PdfSignature field
            std::vector<PdfSigner*> Signers;
            std::vector<std::shared_ptr<PdfSigner>> SignersStorage; // Unnecessary for PoDoFo::SignDocument()
        };

        struct SignatureCtx
        {
            charbuff Contents;
            size_t BeaconSize = 0;
            PdfSignatureBeacons Beacons;
            PdfArray ByteRangeArr;
        };

    private:
        // Used by PoDoFo::SignDocument
        void AddSignerUnsafe(const PdfSignature& signature, PdfSigner& signer);

    private:
        PdfSignerId addSigner(const PdfSignature& signature, PdfSigner* signer,
            const std::shared_ptr<PdfSigner>& storage);
        void ensureNotStarted() const;
        std::unordered_map<PdfSignerId, SignatureCtx> prepareSignatureContexts(PdfDocument& doc, bool sequentialSigning);
        void saveDocForSigning(PdfMemDocument& doc, StreamDevice& device);
        void appendDataForSigning(std::unordered_map<PdfSignerId, SignatureCtx>& contexts, StreamDevice& device,
            std::unordered_map<PdfSignerId, charbuff>* intermediateResults, charbuff& tmpbuff);
        void computeSignatures(std::unordered_map<PdfSignerId, SignatureCtx>& contexts,
            PdfDocument& doc, StreamDevice& device,
            const std::unordered_map<PdfSignerId, charbuff>* processedResults, charbuff& tmpbuff);

    private:
        PdfSigningContext(const PdfSigningContext&) = delete;
        PdfSigningContext& operator==(const PdfSigningContext&) = delete;

    private:
        PdfSaveOptions m_SaveOptions;
        std::unordered_map<PdfReference, SignatureAttrs> m_signers;
        // Used during sequential signing
        PdfMemDocument* m_doc;
        std::shared_ptr<StreamDevice> m_device;
        std::unordered_map<PdfSignerId, SignatureCtx> m_contexts;
    };
}

#endif // PDF_SIGNING_CONTEXT_H
