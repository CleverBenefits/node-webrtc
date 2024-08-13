/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_audio_video_source.h"

#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/video/i420_buffer.h>
#include <webrtc/api/video/video_frame.h>
#include <webrtc/rtc_base/ref_counted_object.h>

#include "src/converters.h"
#include "src/converters/absl.h"
#include "src/converters/arguments.h"
#include "src/converters/napi.h"
#include "src/dictionaries/webrtc/video_frame_buffer.h"
#include "src/functional/maybe.h"
#include "src/interfaces/media_stream_track.h"

#include "src/interfaces/rtc_audio_source.h"
#include "src/interfaces/rtc_video_source.h"

#include <chrono>
#include <ctime>

namespace node_webrtc {

Napi::FunctionReference &RTCAudioVideoSource::constructor() {
  static Napi::FunctionReference constructor;
  return constructor;
}

RTCAudioVideoSource::RTCAudioVideoSource(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<RTCAudioVideoSource>(info) {
  New(info);
}

Napi::Value RTCAudioVideoSource::New(const Napi::CallbackInfo &info) {
  auto env = info.Env();

  if (!info.IsConstructCall()) {
    Napi::TypeError::New(env,
                         "Use the new operator to construct an RTCAudioVideoSource.")
        .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, maybeInit,
                                        Maybe<RTCVideoSourceInit>)
  auto init = maybeInit.FromMaybe(RTCVideoSourceInit());

  auto needsDenoising = init.needsDenoising
                            .Map([](auto needsDenoising) {
                              return absl::optional<bool>(needsDenoising);
                            })
                            .FromMaybe(absl::optional<bool>());

  _videoSource = new rtc::RefCountedObject<RTCVideoTrackSource>(init.isScreencast,
                                                                needsDenoising);
  _audioSource = rtc::make_ref_counted<RTCAudioTrackSource>();

  return info.Env().Undefined();
}

Napi::Value RTCAudioVideoSource::CreateVideoTrack(const Napi::CallbackInfo &info) {
  auto factory = PeerConnectionFactory::GetOrCreateDefault();
  auto track =
      factory->factory()->CreateVideoTrack(rtc::CreateRandomUuid(), _videoSource);
  return MediaStreamTrack::wrap()->GetOrCreate(factory, track)->Value();
}

Napi::Value RTCAudioVideoSource::CreateAudioTrack(const Napi::CallbackInfo &info) {
  auto factory = PeerConnectionFactory::GetOrCreateDefault();
  auto track =
      factory->factory()->CreateAudioTrack(rtc::CreateRandomUuid(), _audioSource);
  return MediaStreamTrack::wrap()->GetOrCreate(factory, track)->Value();
}

Napi::Value RTCAudioVideoSource::OnFrame(const Napi::CallbackInfo &info) {
  CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, buffer,
                                        rtc::scoped_refptr<webrtc::I420Buffer>)

  auto now = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now());
  uint64_t nowInUs = now.time_since_epoch().count();

  webrtc::VideoFrame::Builder builder;
  auto frame = builder.set_timestamp_us(static_cast<int64_t>(nowInUs))
                   .set_video_frame_buffer(buffer)
                   .build();
  _videoSource->PushFrame(frame);
  return info.Env().Undefined();
}

Napi::Value RTCAudioVideoSource::OnData(const Napi::CallbackInfo &info) {
  CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, dict, RTCOnDataEventDict)
  _audioSource->PushData(dict);
  return info.Env().Undefined();
}

Napi::Value RTCAudioVideoSource::GetNeedsDenoising(const Napi::CallbackInfo &info) {
  CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _videoSource->needs_denoising(),
                                   result, Napi::Value)
  return result;
}

Napi::Value RTCAudioVideoSource::GetIsScreencast(const Napi::CallbackInfo &info) {
  CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _videoSource->is_screencast(), result,
                                   Napi::Value)
  return result;
}

void RTCAudioVideoSource::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "RTCAudioVideoSource",
      {InstanceMethod("createVideoTrack", &RTCAudioVideoSource::CreateVideoTrack),
       InstanceMethod("createAudioTrack", &RTCAudioVideoSource::CreateAudioTrack),
       InstanceMethod("onFrame", &RTCAudioVideoSource::OnFrame),
       InstanceMethod("onData", &RTCAudioVideoSource::OnData),
       InstanceAccessor("needsDenoising", &RTCAudioVideoSource::GetNeedsDenoising,
                        nullptr),
       InstanceAccessor("isScreencast", &RTCAudioVideoSource::GetIsScreencast,
                        nullptr)});

  constructor() = Napi::Persistent(func);
  constructor().SuppressDestruct();

  exports.Set("RTCAudioVideoSource", func);
}

} // namespace node_webrtc
