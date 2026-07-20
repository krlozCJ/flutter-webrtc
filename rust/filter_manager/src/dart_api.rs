// dart_api.rs
use crate::filter_manager::{FilterCommand, send_command};
use crate::gpu_filter::FilterConfig;

pub fn add_filter(track_id: String, filter: String) -> bool {
    let config = default_config_for(&filter);
    send_command(&track_id, FilterCommand::Add(filter, config))
}

pub fn remove_filter(track_id: String, filter: String) -> bool {
    send_command(&track_id, FilterCommand::Remove(filter))
}

pub fn set_param(track_id: String, filter: String, config: FilterConfig) -> bool {
    send_command(&track_id, FilterCommand::Configure(filter, config))
}

// Config por defecto al agregar un filtro sin parámetros específicos desde Dart todavía
fn default_config_for(filter: &str) -> FilterConfig {
    match filter {
        "color_correction" => FilterConfig::ColorCorrection {
            brightness: 0.0,
            contrast: 1.0,
            saturation: 1.0,
        },
        "beauty" => FilterConfig::Beauty { smoothing: 0.5 },
        "cinematic" => FilterConfig::Cinematic { lut_index: 0 },
        "privacy" => FilterConfig::Privacy { blur_strength: 0.5 },
        _ => panic!("Filtro desconocido: {}", filter),
    }
}
