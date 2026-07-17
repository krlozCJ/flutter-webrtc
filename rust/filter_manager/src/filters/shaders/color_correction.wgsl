@group(0) @binding(0) var input_tex: texture_2d<f32>;
@group(0) @binding(1) var output_tex: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = textureLoad(input_tex, vec2<i32>(id.xy), 0);
    // aquí iría tu lógica real de brightness/contrast/saturation
    textureStore(output_tex, vec2<i32>(id.xy), color);
}