#ifndef FLUTTER_WEBRTC_RTC_CUSTOM_TRACK_HXX
#define FLUTTER_WEBRTC_RTC_CUSTOM_TRACK_HXX
#include "flutter_common.h"
#include "flutter_webrtc_base.h"

#include "rtc_video_frame.h"
#include "rtc_video_renderer.h"
#include "rtc_video_source.h"
#include "rtc_mediaconstraints.h"

#include <mutex>
#include <vector>

namespace flutter_webrtc_plugin{
using namespace libwebrtc;

class FlutterProxyRenderer
    : public RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>,
      public RefCountInterface {

    FlutterProxyRenderer() = default;
    ~FlutterProxyRenderer();

    void Initialize(scoped_refptr<RTCVideoSource> source);

    virtual void OnFrame(scoped_refptr<RTCVideoFrame> frame) override;

    void SetTrack(scoped_refptr<RTCVideoTrack> track);

    std::string track_id;

    private:
        mutable std::vector<uint8_t> rgb_buffer_;
        mutable std::mutex mutex_;
        scoped_refptr<RTCVideoTrack> track_ = nullptr;
        scoped_refptr<RTCVideoSource> source_ = nullptr;

};

class FlutterCustomTrackManager {
    public:
        FlutterCustomTrackManager(FlutterWebRTCBase* base);

        void CreateSintheticTrack(
            const std::string& tagName,
            std::unique_ptr<MethodResultProxy> result
        );
        void AttachToTrack(
            const std::string& sinthetic_track_id,
            const std::string& track_id,
            std::unique_ptr<MethodResultProxy> result
        );
        void UnAttachToTrack(const std::string& sinthetic_track_id);

        void AutoUnAttachToTrack(scoped_refptr<RTCVideoTrack> track);

        bool IsAttachedToProxy(scoped_refptr<RTCVideoTrack> track);

        void SintheticTrackDispose(
            const std::string& sinthetic_track_id
        );
        void SintheticTrackDispose(
            const std::string& sinthetic_track_id,
            std::unique_ptr<MethodResultProxy> result
        );

        private:
            FlutterWebRTCBase* base_;
            std::map<std::string, scoped_refptr<FlutterProxyRenderer>> renderers_;
            std::map<std::string, scoped_refptr<RTCVideoSource>> sinthetic_tracks_;
            std::map<std::string, scoped_refptr<RTCVideoTrack>> origin_tracks_;
};
}
