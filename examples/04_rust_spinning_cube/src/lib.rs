#![no_std]

use core::cell::UnsafeCell;
use core::ffi::c_void;
use core::mem::size_of;
use core::panic::PanicInfo;
use core::ptr;
use zsdk::core_sys::DebugOverlay;
use zsdk::graphics::{BlendMode, GLContext, GLContextConfig, PrimitiveType, renderer};
use zsdk::log;
use zsdk::sys::{
    ZsdkButton, zsdk_input_is_down, zsdk_input_is_pressed,
    zsdk_input_left_stick, zsdk_input_right_stick, zsdk_input_t,
};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

struct SyncCell<T>(UnsafeCell<T>);
unsafe impl<T> Sync for SyncCell<T> {}

#[repr(C)]
struct CubeVertex {
    x: f32,
    y: f32,
    z: f32,
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

#[rustfmt::skip]
static CUBE_VERTICES: [CubeVertex; 24] = [
    // Front (+Z) - Cyan
    CubeVertex { x: -0.8, y: -0.8, z:  0.8, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x:  0.8, y: -0.8, z:  0.8, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z:  0.8, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x: -0.8, y:  0.8, z:  0.8, r:  60, g: 180, b: 255, a: 255 },
    // Back (-Z) - Orange
    CubeVertex { x:  0.8, y: -0.8, z: -0.8, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x: -0.8, y: -0.8, z: -0.8, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x: -0.8, y:  0.8, z: -0.8, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z: -0.8, r: 255, g: 140, b:  50, a: 255 },
    // Left (-X) - Green
    CubeVertex { x: -0.8, y: -0.8, z: -0.8, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -0.8, y: -0.8, z:  0.8, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -0.8, y:  0.8, z:  0.8, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -0.8, y:  0.8, z: -0.8, r:  70, g: 220, b: 120, a: 255 },
    // Right (+X) - Magenta
    CubeVertex { x:  0.8, y: -0.8, z:  0.8, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  0.8, y: -0.8, z: -0.8, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z: -0.8, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z:  0.8, r: 230, g:  80, b: 190, a: 255 },
    // Top (+Y) - Yellow
    CubeVertex { x: -0.8, y:  0.8, z:  0.8, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z:  0.8, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x:  0.8, y:  0.8, z: -0.8, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x: -0.8, y:  0.8, z: -0.8, r: 250, g: 220, b:  60, a: 255 },
    // Bottom (-Y) - Red
    CubeVertex { x: -0.8, y: -0.8, z: -0.8, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x:  0.8, y: -0.8, z: -0.8, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x:  0.8, y: -0.8, z:  0.8, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x: -0.8, y: -0.8, z:  0.8, r: 235, g:  65, b:  65, a: 255 },
];

#[rustfmt::skip]
static CUBE_INDICES: [u16; 36] = [
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23,
];

struct CubeState {
    gl: Option<GLContext>,
    angle_y: f32,
    angle_x: f32,
    pan_x: f32,
    pan_y: f32,
    cam_dist: f32,
    auto_spin: bool,
}

static STATE: SyncCell<CubeState> = SyncCell(UnsafeCell::new(CubeState {
    gl: None,
    angle_y: 0.0,
    angle_x: 0.0,
    pan_x: 0.0,
    pan_y: 0.0,
    cam_dist: 4.0,
    auto_spin: true,
}));

#[inline]
fn clampf(v: f32, lo: f32, hi: f32) -> f32 {
    if v < lo { lo } else if v > hi { hi } else { v }
}

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
            log::error(c"CUBE_RUST", c"GLContext init failed");
            return -1;
        }
    };
    if gl.make_current().is_err() {
        return -1;
    }

    renderer::reset_state_cache();
    renderer::set_depth_test(true);
    renderer::set_cull_face(true);
    renderer::set_lighting(false);
    DebugOverlay::set_visible(true);

    (*STATE.0.get()).gl = Some(gl);
    log::info(c"CUBE_RUST", c"Ready");
    0
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_update(delta_ms: u32, input: *const zsdk_input_t) {
    let dt_sec = (delta_ms as f32) * 0.001;
    let st = &mut *STATE.0.get();

    if st.auto_spin {
        st.angle_y += dt_sec * 48.0;
        st.angle_x += dt_sec * 24.0;
    }

    if !input.is_null() {
        let (mut lx, mut ly) = (0.0f32, 0.0f32);
        let (mut rx, mut ry) = (0.0f32, 0.0f32);
        zsdk_input_left_stick(input, 0, &mut lx, &mut ly);
        zsdk_input_right_stick(input, 0, &mut rx, &mut ry);

        if zsdk_input_is_down(input, 0, ZsdkButton::DPadLeft)  != 0 { lx -= 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadRight) != 0 { lx += 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadUp)    != 0 { ly -= 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadDown)  != 0 { ly += 1.0; }

        st.pan_x = clampf(st.pan_x + lx * 4.0 * dt_sec, -6.0, 6.0);
        st.pan_y = clampf(st.pan_y - ly * 4.0 * dt_sec, -4.0, 4.0);

        st.angle_y += rx * 120.0 * dt_sec;

        let mut zoom = -ry * 6.0;
        if zsdk_input_is_down(input, 0, ZsdkButton::ShoulderLeft)  != 0 { zoom -= 6.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::ShoulderRight) != 0 { zoom += 6.0; }
        st.cam_dist = clampf(st.cam_dist + zoom * dt_sec, 2.2, 20.0);

        if zsdk_input_is_pressed(input, 0, ZsdkButton::Action1) != 0 {
            st.auto_spin = !st.auto_spin;
            log::info(c"CUBE_RUST", if st.auto_spin { c"Auto-spin: ON" } else { c"Auto-spin: OFF" });
        }
        if zsdk_input_is_pressed(input, 0, ZsdkButton::Action2) != 0 {
            st.pan_x = 0.0;
            st.pan_y = 0.0;
            st.cam_dist = 4.0;
            log::info(c"CUBE_RUST", c"Camera reset");
        }
    }

    if st.angle_y >= 360.0 { st.angle_y -= 360.0; }
    if st.angle_x >= 360.0 { st.angle_x -= 360.0; }
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_render(out_width: *mut i32, out_height: *mut i32) {
    let st = &*STATE.0.get();
    let gl = match st.gl.as_ref() {
        Some(g) => g,
        None => return,
    };
    let w = gl.width();
    let h = gl.height();
    let aspect = if h > 0 { (w as f32) / (h as f32) } else { 1.333 };

    renderer::set_viewport(0, 0, w, h);
    renderer::set_depth_test(true);
    renderer::set_cull_face(true);
    renderer::set_blend_mode(BlendMode::Opaque);
    renderer::clear(20, 24, 34, 255, true, false);

    renderer::set_perspective(45.0, aspect, 0.1, 50.0);
    renderer::reset_modelview();
    renderer::look_at(
        [st.pan_x, 1.2 + st.pan_y, st.cam_dist],
        [st.pan_x, st.pan_y, 0.0],
        [0.0, 1.0, 0.0],
    );

    renderer::draw_blob_shadow(0.0, -1.25, 0.0, 1.15, [0, 0, 0, 110]);

    renderer::push_matrix();
    renderer::rotate(st.angle_y, 0.0, 1.0, 0.0);
    renderer::rotate(st.angle_x, 1.0, 0.0, 0.0);
    let pos_ptr = ptr::addr_of!(CUBE_VERTICES[0].x) as *const c_void;
    let col_ptr = ptr::addr_of!(CUBE_VERTICES[0].r) as *const c_void;
    renderer::bind_raw_vertex_arrays(
        pos_ptr,
        3,
        ptr::null(),
        col_ptr,
        ptr::null(),
        size_of::<CubeVertex>() as i32,
    );
    renderer::draw_elements_u16(PrimitiveType::Triangles, &CUBE_INDICES);
    renderer::unbind_vertex_attributes();
    renderer::pop_matrix();

    DebugOverlay::record_draw_call(24);

    if !out_width.is_null() { *out_width = w; }
    if !out_height.is_null() { *out_height = h; }
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
