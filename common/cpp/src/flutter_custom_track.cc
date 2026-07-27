#include "flutter_custom_track.h"
#include <windows.h>
#include <thread>
#include <iostream>

namespace flutter_webrtc_plugin {
    // Asegúrate de liberar la memoria en el destructor de FlutterProxyRenderer
    FlutterProxyRenderer::~FlutterProxyRenderer() {
        if (csharp_filter_manager_) {
            RemoveManager(csharp_filter_manager_);
            csharp_filter_manager_ = nullptr;
        }
    }

    void FlutterProxyRenderer::Initialize(
        scoped_refptr<RTCVideoSource> source
    ){
        source_ = source;

        // Inicializamos el manager de C# (retorna el puntero GCHandle)
        if (!csharp_filter_manager_) {
            csharp_filter_manager_ = RegisterManager();
            // Aplicamos el filtro inicial pasando un JSON
            // Nota: Asegúrate de que tu C# procese esta estructura.
            std::string initial_config = "{\"colore\": {\"type\": \"color\", \"contrast\": 1.0, \"brightness\": 0.0, \"saturation\": 1.0, \"temperature\": 0}, \"belleza\": {\"type\": \"beauty\",  \"intensity\": 1.5}}";
            ApplyFilter(csharp_filter_manager_, initial_config.c_str());
        }
    }

//     // Función que será ejecutada en el hilo secundario
// void mostrarAlertaNoBloqueante(std::string mensaje, std::string titulo) {
//     // MB_OK es el tipo de botón, MB_ICONINFORMATION es el icono
//     MessageBoxA(NULL, mensaje.c_str(), titulo.c_str(), MB_OK | MB_ICONINFORMATION);
// }
    void FlutterProxyRenderer::OnFrame(scoped_refptr<RTCVideoFrame> frame){
        std::lock_guard<std::mutex> lock(mutex_);

        int width = frame->width();
        int height = frame->height();
        size_t required_rgba_size = width * height * 4;

        if(rgba_buffer_.size() != required_rgba_size){
            rgba_buffer_.resize(required_rgba_size);
        }

        // Convertir de formato WebRTC a ARGB (en memoria RGBA/ABGR)
        frame->ConvertToARGB(
            RTCVideoFrame::Type::kABGR,
            rgba_buffer_.data(),
            0,
            width,
            height
        );

        // --- PROCESAMIENTO C# ---
        // Le pasamos el puntero a C# para que modifique rgba_buffer_ in-place
        if (csharp_filter_manager_) {
            ProcessFrame(csharp_filter_manager_, rgba_buffer_.data(), width, height);
        }

        // --- Reconstrucción I420 ---
        size_t required_i420_size = (width * height * 3) / 2;
        if(i420_buffer_.size() != required_i420_size){
            i420_buffer_.resize(required_i420_size);
        }

        int stride_y = width;
        int stride_uv = (width + 1) / 2;
        uint8_t* data_y = i420_buffer_.data();
        uint8_t* data_u = data_y + stride_y * height;
        uint8_t* data_v = data_u + stride_uv * ((height + 1) / 2);

        libyuv::ABGRToI420(
            rgba_buffer_.data(), width * 4,
            data_y, stride_y,
            data_u, stride_uv,
            data_v, stride_uv,
            width, height
        );

        scoped_refptr<RTCVideoFrame> new_frame = RTCVideoFrame::Create(
            width, height,
            data_y, stride_y,
            data_u, stride_uv,
            data_v, stride_uv
        );

        source_->OnCapturedFrame(new_frame);
    }

    void FlutterProxyRenderer::SetTrack(scoped_refptr<RTCVideoTrack> track){
        if(track_ != track){
            if(track_){
                // std::thread hiloAlerta(mostrarAlertaNoBloqueante, "Se libera el track anterior", track_->id().std_string());
                // hiloAlerta.detach();
                track_->RemoveRenderer(this);
                track_id.clear();
            }
            track_ = track;
            if(track_){
                std::cout << "Se añade el render correctamente." << std::endl;
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
        result->Success(EncodableValue(params));
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
                    result->Success(EncodableValue("Verdadero"));
                    origin_tracks_[track_id] = video_track;
                    std::cout << "Se asignó correctamente." << std::endl;
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
            }
            renderer->SetTrack(nullptr);
            renderers_.erase(it);
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
            }
            renderer->SetTrack(nullptr);
            renderers_.erase(it_renderer);
        }
    }
}