#ifndef FLUTTER_WEBRTC_RTC_VIDEO_RENDERER_HXX
#define FLUTTER_WEBRTC_RTC_VIDEO_RENDERER_HXX

#include "flutter_common.h"
#include "flutter_webrtc_base.h"

#include "rtc_video_frame.h"
#include "rtc_video_renderer.h"
#include "rtc_video_source.h"
#include "rtc_mediaconstraints.h"

#include <mutex>

namespace flutter_webrtc_plugin {
using namespace libwebrtc;

class FlutterPipeVideoRenderer
    : public RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>,
      public RefCountInterface {
        public:
            FlutterPipeVideoRenderer() = default;
            ~FlutterPipeVideoRenderer();

            void Initialize(scoped_refptr<RTCVideoSource> source, scoped_refptr<RTCVideoTrack> track);
            virtual void OnFrame(scoped_refptr<RTCVideoFrame> frame) override;
            void SetVideoTrack(scoped_refptr<RTCVideoTrack> track);
            scoped_refptr<RTCVideoTrack> GetSyntheticTrack(){
                return synthetic_track_;
            }
        private:
            scoped_refptr<RTCVideoSource> source_ = nullptr;
            scoped_refptr<RTCVideoTrack> track_ = nullptr;
            scoped_refptr<RTCVideoTrack> sinthetic_track_ = nullptr;
            mutable std::mutex mutex_;
      };

class FlutterPipeVideoRendererManager {
    public:
        FlutterPipeVideoRendererManager(FlutterWebRTCBase* base);

        void CreateVideoTrackProxy(scoped_refptr<RTCVideoTrack> track);
    private:
        FlutterWebRTCBase* base_;
        std::map<std::string, scoped_refptr<FlutterPipeVideoRenderer>> renderers_;
        std::map<std::string, scoped_refptr<RTCVideoTrack>> tracks_;
};
}  // namespace flutter_webrtc_plugin

#endif  // !FLUTTER_WEBRTC_RTC_VIDEO_RENDERER_HXX