use crate::gpu_context::GpuContext;
use crate::gpu_filter::{GpuFilter, FilterConfig};

pub struct ColorCorrectionFilter {
    brightness: f32,
    contrast: f32,
    saturation: f32,
    pipeline: wgpu::ComputePipeline,
    bind_group_layout: wgpu::BindGroupLayout,
}

impl ColorCorrectionFilter {
    pub fn new(config: FilterConfig, ctx: &GpuContext) -> Self {
        let shader = ctx.device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("color_correction"),
            source: wgpu::ShaderSource::Wgsl(include_str!("shaders/color_correction.wgsl").into()),
        });

        let bind_group_layout = ctx.device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: None,
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::COMPUTE,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: false },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::COMPUTE,
                    ty: wgpu::BindingType::StorageTexture {
                        access: wgpu::StorageTextureAccess::WriteOnly,
                        format: wgpu::TextureFormat::Rgba8Unorm,
                        view_dimension: wgpu::TextureViewDimension::D2,
                    },
                    count: None,
                },
            ],
        });

        let pipeline_layout = ctx.device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: None,
            bind_group_layouts: &[Some(&bind_group_layout)],
            immediate_size: 0
        });

        let pipeline = ctx.device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
            label: None,
            layout: Some(&pipeline_layout),
            module: &shader,
            entry_point: Some("main"),
            compilation_options: Default::default(),
            cache: None,
        });

        let mut filter = Self { brightness: 0.0, contrast: 1.0, saturation: 1.0, pipeline, bind_group_layout };
        filter.configure(config);
        filter
    }
}

impl GpuFilter for ColorCorrectionFilter {
    fn name(&self) -> &'static str { "color_correction" }

    fn configure(&mut self, config: FilterConfig) {
        if let FilterConfig::ColorCorrection { brightness, contrast, saturation } = config {
            self.brightness = brightness;
            self.contrast = contrast;
            self.saturation = saturation;
        }
    }

    fn apply(&mut self, ctx: &GpuContext, input: &wgpu::TextureView, output: &wgpu::TextureView) {
        let bind_group = ctx.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(input) },
                wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::TextureView(output) },
            ],
        });

        let mut encoder = ctx.device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
        {
            let mut pass = encoder.begin_compute_pass(&wgpu::ComputePassDescriptor::default());
            pass.set_pipeline(&self.pipeline);
            pass.set_bind_group(0, &bind_group, &[]);
            pass.dispatch_workgroups((ctx.width + 7) / 8, (ctx.height + 7) / 8, 1); // ajusta según tu resolución real
        }
        ctx.queue.submit(Some(encoder.finish()));
    }
}