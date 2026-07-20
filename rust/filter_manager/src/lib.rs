mod filter_manager;
mod dart_api;
mod gpu_context;
mod gpu_filter;
mod filters;

use filter_manager::FilterManager;

#[cxx::bridge(namespace = "filter_manager")]
mod ffi {
    extern "Rust" {
        type FilterManager;
        fn create_filter_manager(track_id: &str, width: u32, height: u32) -> Box<FilterManager>;
        fn process_frame_inplace(self: &mut FilterManager, ptr: usize, width: u32, height: u32, stride: u32);
        fn add_color_correction(self: &mut FilterManager);
        fn set_color_correction(self: &mut FilterManager, brightness: f32, contrast: f32, saturation: f32);
        fn destroy_filter_manager(track_id: &str);
    }
}

fn create_filter_manager(track_id: &str, width: u32, height: u32) -> Box<FilterManager> {
    FilterManager::new(track_id, width, height)
}

fn destroy_filter_manager(track_id: &str) {
    filter_manager::registry_remove(track_id);
}