#![no_std]

use core::cell::UnsafeCell;
use core::ffi::c_void;
use core::panic::PanicInfo;
use core::ptr;
use zsdk::graphics::{BlendMode, GLContext, GLContextConfig, PrimitiveType, renderer};
use zsdk::log;
use zsdk::sys::{zsdk_input_left_stick, zsdk_input_t};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

struct SyncCell<T>(UnsafeCell<T>);
unsafe impl<T> Sync for SyncCell<T> {}

struct CubeState {
    gl: Option<GLContext>,
    angle_x: f32,
    angle_y: f32,
}

static STATE: SyncCell<CubeState> = SyncCell(UnsafeCell::new(CubeState {
    gl: None,
    angle_x: 20.0,
    angle_y: 0.0,
}));

#[rustfmt::skip]
static CUBE_POS: [f32; 24] = [
    -0.5, -0.5,  0.5,   0.5, -0.5,  0.5,   0.5,  0.5,  0.5,  -0.5,  0.5,  0.5,
    -0.5, -0.5, -0.5,  -0.5,  0.5, -0.5,   0.5,  0.5, -0.5,   0.5, -0.5, -0.5,
];

#[rustfmt::skip]
static CUBE_COL: [u8; 32] = [
    255,  80,  80, 255,   80, 255,  80, 255,   80,  80, 255, 255,  255, 255,  80, 255,
    255,  80, 255, 255,   80, 255, 255, 255,  255, 160,  80, 255,  255, 255, 255, 255,
];

#[rustfmt::skip]
static CUBE_IDX: [u16; 36] = [
    0, 1, 2,  0, 2, 3,
    4, 5, 6,  4, 6, 7,
    3, 2, 6,  3, 6, 5,
    0, 4, 7,  0, 7, 1,
    1, 7, 6,  1, 6, 2,
    0, 3, 5,  0, 5, 4,
];

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_init(platform_ctx: *mut c_void) -> i32 {
    let cfg = GLContextConfig {
        red_bits: 5,
        green_bits: 6,
        blue_bits: 5,
        alpha_bits: 0,
        depth_bits: 16,
        stencil_bits: 0,
        swap_interval: 1,
    };
    let mut gl = match GLContext::new(platform_ctx, Some(&cfg)) {
        Ok(ctx) => ctx,
        Err(_) => {
            log::error(c"RUST_CUBE", c"Falha ao criar GLContext");
            return -1;
        }
    };
    if gl.make_current().is_err() {
        return -1;
    }
    (*STATE.0.get()).gl = Some(gl);
    log::info(c"RUST_CUBE", c"Rust Spinning Cube inicializado");
    0
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_update(delta_ms: u32, input: *const zsdk_input_t) {
    let dt_sec = (delta_ms as f32) * 0.001;
    let mut speed_y = 45.0f32;
    let mut speed_x = 18.0f32;
    if !input.is_null() {
        let (mut lx, mut ly) = (0.0f32, 0.0f32);
        zsdk_input_left_stick(input, 0, &mut lx, &mut ly);
        speed_y += lx * 140.0;
        speed_x += ly * 140.0;
    }
    let st = &mut *STATE.0.get();
    st.angle_y += speed_y * dt_sec;
    st.angle_x += speed_x * dt_sec;
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_render(out_w: *mut i32, out_h: *mut i32) {
    let st = &*STATE.0.get();
    let gl = match st.gl.as_ref() {
        Some(g) => g,
        None => return,
    };
    let w = gl.width();
    let h = gl.height().max(1);
    if !out_w.is_null() { *out_w = w; }
    if !out_h.is_null() { *out_h = h; }

    renderer::set_viewport(0, 0, w, h);
    renderer::clear(16, 24, 48, 255, true, false);
    renderer::set_depth_test(true);
    renderer::set_cull_face(true);
    renderer::set_blend_mode(BlendMode::Opaque);
    renderer::set_perspective(45.0, w as f32 / h as f32, 0.1, 100.0);
    renderer::reset_modelview();
    renderer::look_at([0.0, 0.0, 3.2], [0.0, 0.0, 0.0], [0.0, 1.0, 0.0]);
    renderer::rotate(st.angle_x, 1.0, 0.0, 0.0);
    renderer::rotate(st.angle_y, 0.0, 1.0, 0.0);

    renderer::bind_raw_vertex_arrays(
        CUBE_POS.as_ptr() as *const _,
        3,
        ptr::null(),
        CUBE_COL.as_ptr() as *const _,
        ptr::null(),
        0,
    );
    renderer::draw_elements_u16(PrimitiveType::Triangles, &CUBE_IDX);
    renderer::unbind_vertex_attributes();

    renderer::begin_2d(w, h);
    renderer::draw_text_2d(12.0, 12.0, 1.0, c"ZSDK Rust Cube", [255, 255, 255, 255]);
    renderer::end_2d();
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_swap() {
    if let Some(gl) = (*STATE.0.get()).gl.as_mut() {
        let _ = gl.swap_buffers();
    }
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_shutdown() {
    (*STATE.0.get()).gl = None;
}
