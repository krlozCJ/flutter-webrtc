use crate::gpu_context::GpuContext;

pub enum FilterConfig {
    ColorCorrection { brightness: f32, contrast: f32, saturation: f32 },
    Beauty { smoothing: f32 },
    Cinematic { lut_index: usize },
    Privacy { blur_strength: f32 },
}

pub trait GpuFilter: Send {
    fn name(&self) -> &'static str;
    fn configure(&mut self, config: FilterConfig);
    fn apply(&mut self, ctx: &GpuContext, input: &wgpu::TextureView, output: &wgpu::TextureView);
}

// Esta función es la que responde tu pregunta: "¿cómo se instancia cada filtro?"
pub fn create_filter(name: &str, config: FilterConfig, ctx: &GpuContext) -> Box<dyn GpuFilter> {
    match name {
        "color_correction" => Box::new(crate::filters::color_correction::ColorCorrectionFilter::new(config, ctx)),
        "beauty" => Box::new(crate::filters::beauty::BeautyFilter::new(config, ctx)),
        // "cinematic" => Box::new(crate::filters::cinematic::CinematicFilter::new(config, ctx)),
        // "privacy" => Box::new(crate::filters::privacy::PrivacyFilter::new(config, ctx)),
        _ => panic!("Filtro desconocido: {}", name),
    }
}