#include "flutter_custom_track.h"

namespace flutter_webrtc_plugin {
    FlutterProxyRenderer::~FlutterProxyRenderer(){}

    void FlutterProxyRenderer::Initialize(
        scoped_refptr<RTCVideoSource> source
    ){
        source_ = source;
    }

    void FlutterProxyRenderer::OnFrame(scoped_refptr<RTCVideoFrame> frame){
        std::lock_guard<std::mutex> lock(mutex_);

        int width = frame->width();
        int height = frame->height();
        size_t required_rgba_size = width * height * 4;

        if(rgba_buffer_.size() != required_rgba_size){
            rgba_buffer_.resize(required_rgba_size);
        }

        frame->ConvertToARGB(
            RTCVideoFrame::Type::kABGR,
            rgba_buffer_.data(),
            0,
            width,
            height
        );

        source_->OnCapturedFrame(frame);
    }

    void FlutterProxyRenderer::SetTrack(scoped_refptr<RTCVideoTrack> track){
        if(track_ != track){
            if(track_){
                track_->RemoveRenderer(this);
                track_id.clear();
            }
            track_ = track;
            if(track_){
                track_->AddRenderer(this);
                track_id = track_->id().std_string();
            }
        }
    }

    // =============================================================
    // =============================================================
    // =============================================================

    // FlutterCustomTrackManager::FlutterCustomTrackManager(FlutterWebRTCBase* base): base_(base) {}

    void FlutterCustomTrackManager::CreateSintheticTrack(
        const std::string& tagName,
        std::unique_ptr<MethodResultProxy> result
    ) {
        scoped_refptr<RTCMediaConstraints> constraints =
            RTCMediaConstraints::Create();

        scoped_refptr<RTCVideoSource> source = base_->factory_->CreateCustomVideoSource(tagName, constraints);
        if(!source){
            result->Error("Fail on Custom Source", "Imposible crear un source");
            return;
        }

        std::string uuid = base_->GenerateUUID();
        scoped_refptr<libwebrtc::RTCVideoTrack> sinthetic_track = base_->factory_->CreateVideoTrack(source, uuid.c_str());
        if(!sinthetic_track){
            result->Error("Fail on Custom Track Source", "Imposible crear un custom track");
            return;
        }

        base_->local_tracks_[sinthetic_track->id().std_string()] = sinthetic_track;
        // Guarda en colección
        sinthetic_tracks_[uuid] = source;

        auto renderer = new RefCountedObject<FlutterProxyRenderer>();
        renderer->Initialize(source);
        renderers_[uuid] = renderer;

        EncodableMap info;
        info[EncodableValue("id")] = EncodableValue(sinthetic_track->id().std_string());
        info[EncodableValue("label")] = EncodableValue(tagName);
        info[EncodableValue("kind")] = EncodableValue(sinthetic_track->kind().std_string());
        info[EncodableValue("enabled")] = EncodableValue(sinthetic_track->enabled());

        EncodableMap settings;
        settings[EncodableValue("deviceId")] =
            EncodableValue(tagName);
        settings[EncodableValue("kind")] = EncodableValue("videoinput");
        settings[EncodableValue("width")] = EncodableValue(0);
        settings[EncodableValue("height")] = EncodableValue(0);
        settings[EncodableValue("frameRate")] = EncodableValue(0);
        info[EncodableValue("settings")] = EncodableValue(settings);

        EncodableMap params;
        params[EncodableValue("track")] = EncodableValue(info);
        result->Success();
    }

    void FlutterCustomTrackManager::AttachToTrack(
        const std::string& sinthetic_track_id,
        const std::string& track_id,
        std::unique_ptr<MethodResultProxy> result
    ){
        for( auto& [t, track] : base_->local_tracks_) {
            if(track && t == track_id){
                auto it = renderers_.find(sinthetic_track_id);
                if(it != renderers_.end()){
                    auto renderer = (*it).second;
                    // TODO: Buscar el renderer, no crearlo
                    scoped_refptr<RTCVideoTrack> video_track = static_cast<RTCVideoTrack*>(track.get());
                    renderer->SetTrack(video_track);
                    result->Success();
                    return;
                }
            }
        }
        result->Error("AttachToTrack", "Unable to attach track");
    }

    // Si el track esta "atado" a un proxy
    bool FlutterCustomTrackManager::IsAttachedToProxy(scoped_refptr<RTCVideoTrack> track){
        for(auto& it : origin_tracks_) {
            auto current = it.second;
            if(current == track){
                return true;
            }
        }

        return false;
    }

    bool FlutterCustomTrackManager::IsSintheticTrack(const std::string& sinthetic_track_id){
        auto it = sinthetic_tracks_.find(sinthetic_track_id);
        if(it != sinthetic_tracks_.end()){
            return true;
        }
        return false;
    };

    // Cuando el track original quiere desacoplarse.
    // Especialmente cuando el track es eliminado y requiere ser liberado
    void FlutterCustomTrackManager::AutoUnAttachToTrack(scoped_refptr<RTCVideoTrack> track) {
        auto track_id = track->id();
        auto it = origin_tracks_.find(track_id.std_string());
        if(it != origin_tracks_.end()){
            for(auto& [sinthetic_track_id, renderer] : renderers_){
                if(renderer && renderer->track_id == track_id.std_string()){
                    renderer->SetTrack(nullptr);
                }
            }
            origin_tracks_.erase(it);
        }
    }

    // Se pasa el id del track sintetico
    void FlutterCustomTrackManager::UnAttachToTrack(
        const std::string& sinthetic_track_id
    ){
        auto it = renderers_.find(sinthetic_track_id);
        if(it != renderers_.end()){
            auto renderer = (*it).second;
            auto track_id = renderer->track_id;
            if(!track_id.empty()){
                auto itt = origin_tracks_.find(track_id);
                if(itt != origin_tracks_.end()){
                    origin_tracks_.erase(itt);
                }
                renderer->SetTrack(nullptr);
                renderers_.erase(it);
            }
        }
    }

    // Cuando explicitamente se libera al track sintetico
    void FlutterCustomTrackManager::SintheticTrackDispose(
        const std::string& sinthetic_track_id
    ){

        auto it_source = sinthetic_tracks_.find(sinthetic_track_id);
        if(it_source != sinthetic_tracks_.end()){
            sinthetic_tracks_.erase(it_source);
        }

        auto it_renderer = renderers_.find(sinthetic_track_id);
        if(it_renderer != renderers_.end()){
            auto renderer = it_renderer->second;
            auto original_track_id = renderer->track_id;

            if(!original_track_id.empty()){
                auto it_origin = origin_tracks_.find(original_track_id);
                if(it_origin != origin_tracks_.end()){
                    origin_tracks_.erase(it_origin);
                }
                renderer->SetTrack(nullptr);
            }

            renderers_.erase(it_renderer);
        }
    }
}