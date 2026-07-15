#include "flutter_pipe_video_renderer.h"

namespace flutter_webrtc_plugin {
    FlutterPipeVideoRenderer::~FlutterPipeVideoRenderer(){}

    void FlutterPipeVideoRenderer::Initialize(scoped_refptr<RTCVideoSource> source, scoped_refptr<RTCVideoTrack> track) {
        source_ = source;
        sinthetic_track_ = track;
        // Acá inicia el binding entre Rust y c++
    }

    void FlutterPipeVideoRenderer::OnFrame(scoped_refptr<RTCVideoFrame> frame){
        std::lock_guard<std::mutex> lock(mutex_);
        // mutex_.lock();

        source_->OnCapturedFrame(frame);

        // mutex_.unlock();
    }

    void FlutterPipeVideoRenderer::SetVideoTrack(scoped_refptr<RTCVideoTrack> track){
        if(track_ != track){
            if(track_){
                track_->RemoveRenderer(this);
            }
            track_ = track;
            if(track_){
                track_->AddRenderer(this);
            }
        }
    }

    // ==================================
    FlutterPipeVideoRendererManager::FlutterPipeVideoRendererManager(
        FlutterWebRTCBase* base
    ) : base_(base) {}

    void FlutterPipeVideoRendererManager::CreateVideoTrackProxy(scoped_refptr<RTCVideoTrack> track){
        auto track_id = track->id().std_string();

        scoped_refptr<RTCMediaConstraints> video_constrains =
            RTCMediaConstraints::Create();

        scoped_refptr<RTCVideoSource> source = base_->factory_->CreateCustomVideoSource("synthetic-video", video_constrains);
        if(!source){
            // result->Error("Fail on Custom Source", "Imposible crear un source");
            return;
        }

        scoped_refptr<libwebrtc::RTCVideoTrack> sinthetic_video_track = base_->factory_->CreateVideoTrack(source, track_id.c_str());
        if(!sinthetic_video_track){
            // result->Error("Fail on Custom Track Source", "Imposible crear un custom track");
            return;
        }

        auto newrenderer = new RefCountedObject<FlutterPipeVideoRenderer>();
        newrenderer->Initialize(source, sinthetic_video_track);
        renderers_[track_id] = newrenderer; // Adding to local
        tracks_[track_id] = track; // Original track
        newrenderer->SetVideoTrack(track);

        base_->local_tracks_[track->id().std_string()] = sinthetic_video_track;

        for(auto& [media_stream_id, media_stream] : base_->local_streams_){
            if(media_stream) {
                auto found_track = media_stream->FindVideoTrack(track_id);

                if(found_track){
                    media_stream->RemoveTrack(static_cast<RTCVideoTrack*>(track.get()));
                    media_stream->AddTrack(static_cast<RTCVideoTrack*>(sinthetic_video_track.get()));
                }
            }
        }

        for(auto& [pc_id, pc] : base_->peerconnections_){
            for(auto& sender : pc->senders()) {
                if(sender->track() && sender->track()->id().std_string() == track_id){
                    sender->set_track(sinthetic_video_track);
                }
            }
        }
    }
}