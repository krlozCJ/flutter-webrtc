// filter_manager.rs
use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::sync::Mutex;
use std::sync::mpsc::{Receiver, Sender, channel};

use crate::gpu_context::GpuContext;
use crate::gpu_filter::{FilterConfig, GpuFilter};

static REGISTRY: Lazy<Mutex<HashMap<String, Sender<FilterCommand>>>> =
    Lazy::new(|| Mutex::new(HashMap::new()));

pub struct FilterManager {
    gpu: GpuContext,
    filters: Vec<Box<dyn GpuFilter>>,
    command_rx: Receiver<FilterCommand>,
}

pub enum FilterCommand {
    Add(String, FilterConfig),
    Remove(String),
    Configure(String, FilterConfig),
}

impl FilterManager {
    pub fn new(track_id: &str, width: u32, height: u32) -> Box<Self> {
        let (tx, rx) = channel();
        REGISTRY.lock().unwrap().insert(track_id.to_string(), tx);
        Box::new(Self {
            gpu: GpuContext::new(width, height),
            filters: Vec::new(),
            command_rx: rx,
        })
    }

    pub fn process_frame_inplace(&mut self, ptr: usize, width: u32, height: u32, stride: u32) {
        while let Ok(cmd) = self.command_rx.try_recv() {
            self.apply_command(cmd);
        }

        let buf =
            unsafe { std::slice::from_raw_parts_mut(ptr as *mut u8, (stride * height) as usize) };

        self.gpu.queue.write_texture(
            self.gpu.texture_a.as_image_copy(),
            buf,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(stride),
                rows_per_image: Some(height),
            },
            wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
        );

        let mut src_is_a = true;
        for filter in &mut self.filters {
            let (src_tex, dst_tex) = if src_is_a {
                (&self.gpu.texture_a, &self.gpu.texture_b)
            } else {
                (&self.gpu.texture_b, &self.gpu.texture_a)
            };
            let src_view = src_tex.create_view(&Default::default());
            let dst_view = dst_tex.create_view(&Default::default());
            filter.apply(&self.gpu, &src_view, &dst_view);
            src_is_a = !src_is_a;
        }

        let final_texture = if src_is_a {
            &self.gpu.texture_a
        } else {
            &self.gpu.texture_b
        };
        self.download_to_buffer(final_texture, buf, width, height, stride);
    }

    fn apply_command(&mut self, cmd: FilterCommand) {
        match cmd {
            FilterCommand::Add(name, cfg) => self
                .filters
                .push(crate::gpu_filter::create_filter(&name, cfg, &self.gpu)),
            FilterCommand::Remove(name) => self.filters.retain(|f| f.name() != name),
            FilterCommand::Configure(name, cfg) => {
                if let Some(f) = self.filters.iter_mut().find(|f| f.name() == name) {
                    f.configure(cfg);
                }
            }
        }
    }

    fn download_to_buffer(
        &self,
        texture_src: &wgpu::Texture,
        out: &mut [u8],
        width: u32,
        height: u32,
        stride: u32,
    ) {
        let buffer_size = (stride * height) as u64;
        let staging_buffer = self.gpu.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("staging_download"),
            size: buffer_size,
            usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
            mapped_at_creation: false,
        });

        let mut encoder = self
            .gpu
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
        encoder.copy_texture_to_buffer(
            texture_src.as_image_copy(),
            wgpu::TexelCopyBufferInfo {
                buffer: &staging_buffer,
                layout: wgpu::TexelCopyBufferLayout {
                    offset: 0,
                    bytes_per_row: Some(stride),
                    rows_per_image: Some(height),
                },
            },
            wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
        );
        self.gpu.queue.submit(Some(encoder.finish()));

        let slice = staging_buffer.slice(..);
        let (tx, rx) = std::sync::mpsc::channel();
        slice.map_async(wgpu::MapMode::Read, move |result| {
            tx.send(result).unwrap();
        });
        self.gpu
            .device
            .poll(wgpu::PollType::Wait {
                submission_index: None,
                timeout: Some(std::time::Duration::from_millis(500)),
            })
            .unwrap();
        rx.recv().unwrap().unwrap();

        let data = slice
            .get_mapped_range()
            .expect("fallo al mapear buffer GPU");
        out.copy_from_slice(&data);
        drop(data);
        staging_buffer.unmap();
    }

    pub fn add_color_correction(&mut self) {
        self.filters.push(crate::gpu_filter::create_filter(
            "color_correction",
            FilterConfig::ColorCorrection {
                brightness: 0.0,
                contrast: 1.0,
                saturation: 1.0,
            },
            &self.gpu,
        ));
    }

    pub fn set_color_correction(&mut self, brightness: f32, contrast: f32, saturation: f32) {
        if let Some(f) = self
            .filters
            .iter_mut()
            .find(|f| f.name() == "color_correction")
        {
            f.configure(FilterConfig::ColorCorrection {
                brightness,
                contrast,
                saturation,
            });
        }
    }
}

pub fn registry_remove(track_id: &str) {
    REGISTRY.lock().unwrap().remove(track_id);
}

pub fn send_command(track_id: &str, cmd: FilterCommand) -> bool {
    REGISTRY
        .lock()
        .unwrap()
        .get(track_id)
        .map(|tx| tx.send(cmd).is_ok())
        .unwrap_or(false)
}
