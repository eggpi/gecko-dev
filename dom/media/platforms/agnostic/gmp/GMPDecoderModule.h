/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(GMPDecoderModule_h_)
#define GMPDecoderModule_h_

#include "PlatformDecoderModule.h"
#include "mozilla/Maybe.h"

namespace mozilla {

class GMPDecoderModule : public PlatformDecoderModule {
public:
  GMPDecoderModule();

  virtual ~GMPDecoderModule();

  // Decode thread.
  already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  // Decode thread.
  already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;

  bool
  SupportsMimeType(const nsACString& aMimeType) override;

  // Main thread only.
  static void Init();

  static const Maybe<nsCString> PreferredGMP(const nsACString& aMimeType);

  static bool SupportsMimeType(const nsACString& aMimeType,
                               const Maybe<nsCString>& aGMP);

  // Main thread only.
  static void UpdateUsableCodecs();
};

} // namespace mozilla

#endif // GMPDecoderModule_h_
