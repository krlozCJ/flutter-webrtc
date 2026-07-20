#ifndef FLUTTER_WEBRTC_RTC_CUSTOM_TRACK_HXX
#define FLUTTER_WEBRTC_RTC_CUSTOM_TRACK_HXX
#include "flutter_common.h"
#include "flutter_webrtc_base.h"

#include "rtc_video_frame.h"
#include "rtc_video_renderer.h"
#include "rtc_video_source.h"
#include "rtc_mediaconstraints.h"
#include "filter_manager/lib.rs.h"
#include <libyuv.h>

#include <mutex>
#include <vector>

namespace flutter_webrtc_plugin {

class FlutterProxyRenderer
    : public RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>,
      public RefCountInterface {

    public:
        FlutterProxyRenderer() = default;
        ~FlutterProxyRenderer();

        void Initialize(scoped_refptr<RTCVideoSource> source, const std::string& track_id);

        virtual void OnFrame(scoped_refptr<RTCVideoFrame> frame) override;

        void SetTrack(scoped_refptr<RTCVideoTrack> track);

        std::string track_id;

    private:
        mutable std::vector<uint8_t> rgba_buffer_;
        mutable std::vector<uint8_t> i420_buffer_; // nuevo
        mutable std::mutex mutex_;
        scoped_refptr<RTCVideoTrack> track_ = nullptr;
        scoped_refptr<RTCVideoSource> source_ = nullptr;

        std::string sinthetic_track_id_;
        std::optional<rust::Box<filter_manager::FilterManager>> filter_manager_;

};

class FlutterCustomTrackManager {
    public:
        FlutterCustomTrackManager(FlutterWebRTCBase* base): base_(base){}

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

        bool IsSintheticTrack(const std::string& sinthetic_track_id);

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
#endif