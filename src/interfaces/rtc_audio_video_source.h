/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <memory>
#include <atomic>


#include <webrtc/pc/local_audio_source.h>

#include <absl/types/optional.h>
#include <node-addon-api/napi.h>
#include <webrtc/api/media_stream_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/media/base/adapted_video_track_source.h>

#include "src/dictionaries/node_webrtc/rtc_on_data_event_dict.h"
#include "src/interfaces/rtc_audio_source.h"
#include "src/interfaces/rtc_video_source.h"

#include "src/dictionaries/node_webrtc/rtc_video_source_init.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"

namespace node_webrtc {

class RTCAudioVideoSource : public Napi::ObjectWrap<RTCAudioVideoSource> {
public:
  explicit RTCAudioVideoSource(const Napi::CallbackInfo &);

  static void Init(Napi::Env, Napi::Object);

private:
  static Napi::FunctionReference &constructor();

  Napi::Value New(const Napi::CallbackInfo &);

  Napi::Value GetIsScreencast(const Napi::CallbackInfo &);
  Napi::Value GetNeedsDenoising(const Napi::CallbackInfo &);

  Napi::Value CreateAudioTrack(const Napi::CallbackInfo &);
  Napi::Value CreateVideoTrack(const Napi::CallbackInfo &);
  Napi::Value OnFrame(const Napi::CallbackInfo &);
  Napi::Value OnData(const Napi::CallbackInfo &);

  rtc::scoped_refptr<RTCVideoTrackSource> _videoSource;
  rtc::scoped_refptr<RTCAudioTrackSource> _audioSource;

};

} // namespace node_webrtc

