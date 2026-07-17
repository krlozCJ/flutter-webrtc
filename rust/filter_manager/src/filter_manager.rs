// filter_manager.rs
use std::collections::HashMap;
use std::sync::Mutex;
use std::sync::mpsc::{channel, Sender, Receiver};
use once_cell::sync::Lazy;

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

        let buf = unsafe {
            std::slice::from_raw_parts_mut(ptr as *mut u8, (stride * height) as usize)
        };

        // 1. Upload CPU -> texture_a
        self.gpu.queue.write_texture(
            self.gpu.texture_a.as_image_copy(),
            buf,
            wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(stride), rows_per_image: Some(height) },
            wgpu::Extent3d { width, height, depth_or_array_layers: 1 },
        );

        // 2. Ping-pong entre filtros
        let mut src = self.gpu.texture_a.create_view(&Default::default());
        let mut dst = self.gpu.texture_b.create_view(&Default::default());
        for filter in &mut self.filters {
            filter.apply(&self.gpu, &src, &dst);
            std::mem::swap(&mut src, &mut dst);
        }

        // 3. Download textura final -> mismo buffer CPU (staging buffer necesario, omitido por brevedad)
        self.download_to_buffer(&src, buf, width, height, stride);
    }

    fn apply_command(&mut self, cmd: FilterCommand) {
        match cmd {
            FilterCommand::Add(name, cfg) => self.filters.push(create_filter(&name, cfg, &self.gpu)),
            FilterCommand::Remove(name) => self.filters.retain(|f| f.name() != name),
            FilterCommand::Configure(name, cfg) => {
                if let Some(f) = self.filters.iter_mut().find(|f| f.name() == name) {
                    f.configure(cfg);
                }
            }
        }
    }
}

pub fn registry_remove(track_id: &str) {
    REGISTRY.lock().unwrap().remove(track_id);
}

pub fn send_command(track_id: &str, cmd: FilterCommand) -> bool {
    REGISTRY.lock().unwrap().get(track_id)
        .map(|tx| tx.send(cmd).is_ok())
        .unwrap_or(false)
}