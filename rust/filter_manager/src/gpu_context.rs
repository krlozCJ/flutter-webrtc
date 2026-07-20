pub struct GpuContext {
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    pub texture_a: wgpu::Texture,
    pub texture_b: wgpu::Texture,
    pub width: u32,
    pub height: u32,
}

impl GpuContext {
    pub fn new(width: u32, height: u32) -> Self {
        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends: wgpu::Backends::DX12,
            flags: wgpu::InstanceFlags::default(),
            backend_options: wgpu::BackendOptions::default(),
            display: None,
            memory_budget_thresholds: wgpu::MemoryBudgetThresholds::default()
        });
        let adapter = pollster::block_on(
            instance.request_adapter(&wgpu::RequestAdapterOptions::default())
        ).unwrap();
        let (device, queue) = pollster::block_on(
            adapter.request_device(&wgpu::DeviceDescriptor::default())
        ).unwrap();

        let make_tex = |device: &wgpu::Device| device.create_texture(&wgpu::TextureDescriptor {
            label: None,
            size: wgpu::Extent3d { width, height, depth_or_array_layers: 1 },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING
                | wgpu::TextureUsages::STORAGE_BINDING
                | wgpu::TextureUsages::COPY_SRC
                | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        let texture_a = make_tex(&device);
        let texture_b = make_tex(&device);

        Self { device, queue, texture_a, texture_b, width, height }
    }
}