// filters/beauty.rs
use crate::gpu_context::GpuContext;
use crate::gpu_filter::{GpuFilter, FilterConfig};

pub struct BeautyFilter { smoothing: f32 }

impl BeautyFilter {
    pub fn new(config: FilterConfig, _ctx: &GpuContext) -> Self {
        let mut f = Self { smoothing: 0.5 };
        f.configure(config);
        f
    }
}

impl GpuFilter for BeautyFilter {
    fn name(&self) -> &'static str { "beauty" }
    fn configure(&mut self, config: FilterConfig) {
        if let FilterConfig::Beauty { smoothing } = config { self.smoothing = smoothing; }
    }
    fn apply(&mut self, _ctx: &GpuContext, _input: &wgpu::TextureView, _output: &wgpu::TextureView) {
        // TODO: implementar
    }
}