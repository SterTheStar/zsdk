#![no_std]

use core::cell::UnsafeCell;
use core::ffi::{CStr, c_void};
use core::mem::size_of;
use core::panic::PanicInfo;
use core::ptr;
use zsdk::core_sys::{self, DebugOverlay};
use zsdk::fs::SaveGameManager;
use zsdk::graphics::{Frustum, GLContext, GLContextConfig, PrimitiveType, SpriteBatch, renderer};
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

static CUBE_COUNTS: [u16; 11] = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024];

#[repr(C)]
struct CubeVertex {
    x: f32,
    y: f32,
    z: f32,
    nx: f32,
    ny: f32,
    nz: f32,
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

#[rustfmt::skip]
static CUBE_VERTICES: [CubeVertex; 24] = [
    // Front (+Z) - Cyan
    CubeVertex { x: -1.0, y: -1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x:  1.0, y: -1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, r:  60, g: 180, b: 255, a: 255 },
    CubeVertex { x: -1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, r:  60, g: 180, b: 255, a: 255 },
    // Back (-Z) - Orange
    CubeVertex { x:  1.0, y: -1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x: -1.0, y: -1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x: -1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, r: 255, g: 140, b:  50, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, r: 255, g: 140, b:  50, a: 255 },
    // Left (-X) - Green
    CubeVertex { x: -1.0, y: -1.0, z: -1.0, nx: -1.0, ny:  0.0, nz:  0.0, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -1.0, y: -1.0, z:  1.0, nx: -1.0, ny:  0.0, nz:  0.0, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -1.0, y:  1.0, z:  1.0, nx: -1.0, ny:  0.0, nz:  0.0, r:  70, g: 220, b: 120, a: 255 },
    CubeVertex { x: -1.0, y:  1.0, z: -1.0, nx: -1.0, ny:  0.0, nz:  0.0, r:  70, g: 220, b: 120, a: 255 },
    // Right (+X) - Magenta
    CubeVertex { x:  1.0, y: -1.0, z:  1.0, nx:  1.0, ny:  0.0, nz:  0.0, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  1.0, y: -1.0, z: -1.0, nx:  1.0, ny:  0.0, nz:  0.0, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z: -1.0, nx:  1.0, ny:  0.0, nz:  0.0, r: 230, g:  80, b: 190, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z:  1.0, nx:  1.0, ny:  0.0, nz:  0.0, r: 230, g:  80, b: 190, a: 255 },
    // Top (+Y) - Yellow
    CubeVertex { x: -1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  1.0, nz:  0.0, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  1.0, nz:  0.0, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x:  1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  1.0, nz:  0.0, r: 250, g: 220, b:  60, a: 255 },
    CubeVertex { x: -1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  1.0, nz:  0.0, r: 250, g: 220, b:  60, a: 255 },
    // Bottom (-Y) - Red
    CubeVertex { x: -1.0, y: -1.0, z: -1.0, nx:  0.0, ny: -1.0, nz:  0.0, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x:  1.0, y: -1.0, z: -1.0, nx:  0.0, ny: -1.0, nz:  0.0, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x:  1.0, y: -1.0, z:  1.0, nx:  0.0, ny: -1.0, nz:  0.0, r: 235, g:  65, b:  65, a: 255 },
    CubeVertex { x: -1.0, y: -1.0, z:  1.0, nx:  0.0, ny: -1.0, nz:  0.0, r: 235, g:  65, b:  65, a: 255 },
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

#[repr(C)]
#[derive(Copy, Clone)]
struct CubeSettings {
    lighting: u8,
    fps_mode: u8,
    vsync: u8,
    spin_mode: u8,
    cube_count_index: u8,
    frustum_cull: u8,
    face_cull: u8,
}

const MENU_ITEM_COUNT: u8 = 10;
const SAVE_VERSION: u32 = 2;

struct CubeState {
    gl: Option<GLContext>,
    batch: Option<SpriteBatch>,
    frustum: Option<Frustum>,
    save_mgr: Option<SaveGameManager>,
    angle_x: f32,
    angle_y: f32,
    cam_pan_x: f32,
    cam_pan_y: f32,
    cam_zoom_offset: f32,
    menu_open: bool,
    menu_cursor: u8,
    settings: CubeSettings,
}

static STATE: SyncCell<CubeState> = SyncCell(UnsafeCell::new(CubeState {
    gl: None,
    batch: None,
    frustum: None,
    save_mgr: None,
    angle_x: 25.0,
    angle_y: 35.0,
    cam_pan_x: 0.0,
    cam_pan_y: 0.0,
    cam_zoom_offset: 0.0,
    menu_open: false,
    menu_cursor: 0,
    settings: CubeSettings {
        lighting: 1,
        fps_mode: if cfg!(target_arch = "arm") { 1 } else { 0 },
        vsync: 1,
        spin_mode: 2,
        cube_count_index: 0,
        frustum_cull: 1,
        face_cull: 1,
    },
}));

#[inline]
fn clampf(v: f32, lo: f32, hi: f32) -> f32 {
    if v < lo { lo } else if v > hi { hi } else { v }
}

fn get_grid_dims(count_idx: u8) -> (u16, u16, f32) {
    const COLS: [u16; 11] = [1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 16];
    const ROWS: [u16; 11] = [1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8];
    const DIST: [f32; 11] = [4.5, 6.5, 8.0, 10.5, 13.0, 16.0, 19.0, 23.0, 28.0, 33.0, 38.0];
    let idx = if count_idx <= 10 { count_idx as usize } else { 10 };
    unsafe { (*COLS.get_unchecked(idx), *ROWS.get_unchecked(idx), *DIST.get_unchecked(idx)) }
}

fn cube_count_at(count_idx: u8) -> u16 {
    let idx = if count_idx <= 10 { count_idx as usize } else { 10 };
    unsafe { *CUBE_COUNTS.get_unchecked(idx) }
}

struct StrBuf<const N: usize> {
    buf: [u8; N],
    len: usize,
}

impl<const N: usize> StrBuf<N> {
    fn new() -> Self {
        Self { buf: [0u8; N], len: 0 }
    }
    fn push_bytes(&mut self, s: &[u8]) {
        for &b in s {
            if self.len + 1 >= N { break; }
            unsafe { *self.buf.get_unchecked_mut(self.len) = b; }
            self.len += 1;
        }
        if self.len < N {
            unsafe { *self.buf.get_unchecked_mut(self.len) = 0; }
        }
    }
    fn push_u32(&mut self, mut val: u32) {
        let mut tmp = [0u8; 12];
        let mut n = 0usize;
        if val == 0 {
            tmp[0] = b'0';
            n = 1;
        } else {
            while val > 0 && n < 11 {
                unsafe { *tmp.get_unchecked_mut(n) = b'0' + (val % 10) as u8; }
                val /= 10;
                n += 1;
            }
        }
        while n > 0 && self.len + 1 < N {
            n -= 1;
            unsafe { *self.buf.get_unchecked_mut(self.len) = *tmp.get_unchecked(n); }
            self.len += 1;
        }
        if self.len < N {
            unsafe { *self.buf.get_unchecked_mut(self.len) = 0; }
        }
    }
    fn as_cstr(&self) -> &CStr {
        let end = if self.len < N { self.len + 1 } else { N };
        unsafe { CStr::from_bytes_with_nul_unchecked(self.buf.get_unchecked(..end)) }
    }
}

fn apply_fps(st: &CubeState) {
    core_sys::set_fps_cap(if (st.settings.fps_mode & 1) != 0 { 30 } else { 60 });
}

fn apply_settings(st: &mut CubeState) {
    apply_fps(st);
    let vsync = if st.settings.vsync != 0 { 1 } else { 0 };
    if let Some(gl) = st.gl.as_mut() {
        let _ = gl.set_swap_interval(vsync);
    }
}

fn adjust_menu_item(st: &mut CubeState, dir: i8) {
    match st.menu_cursor {
        0 => {
            st.settings.cube_count_index = ((st.settings.cube_count_index as i16 + 11 + dir as i16) % 11) as u8;
            st.cam_pan_x = 0.0;
            st.cam_pan_y = 0.0;
            st.cam_zoom_offset = 0.0;
        }
        1 => st.settings.frustum_cull = if st.settings.frustum_cull == 0 { 1 } else { 0 },
        2 => st.settings.face_cull = if st.settings.face_cull == 0 { 1 } else { 0 },
        3 => st.settings.lighting = if st.settings.lighting == 0 { 1 } else { 0 },
        4 => {
            st.settings.fps_mode = if st.settings.fps_mode == 0 { 1 } else { 0 };
            apply_fps(st);
        }
        5 => {
            st.settings.vsync = if st.settings.vsync == 0 { 1 } else { 0 };
            let vsync = if st.settings.vsync != 0 { 1 } else { 0 };
            if let Some(gl) = st.gl.as_mut() {
                let _ = gl.set_swap_interval(vsync);
            }
        }
        6 => st.settings.spin_mode = ((st.settings.spin_mode as i16 + 4 + dir as i16) % 4) as u8,
        7 => DebugOverlay::set_visible(!DebugOverlay::is_visible()),
        8 => {
            let bytes = unsafe {
                core::slice::from_raw_parts(&st.settings as *const CubeSettings as *const u8, size_of::<CubeSettings>())
            };
            if let Some(sg) = st.save_mgr.as_mut() {
                let _ = sg.save_slot(0, bytes, SAVE_VERSION);
                log::info(c"CUBE_RUST", c"Saved settings to udata.");
            }
        }
        9 => core_sys::request_quit(),
        _ => {}
    }
}

unsafe fn update_menu(st: &mut CubeState, input: *const zsdk_input_t) {
    if zsdk_input_is_pressed(input, 0, ZsdkButton::DPadUp) != 0 {
        st.menu_cursor = if st.menu_cursor == 0 { MENU_ITEM_COUNT - 1 } else { st.menu_cursor - 1 };
    }
    if zsdk_input_is_pressed(input, 0, ZsdkButton::DPadDown) != 0 {
        st.menu_cursor = (st.menu_cursor + 1) % MENU_ITEM_COUNT;
    }
    if zsdk_input_is_pressed(input, 0, ZsdkButton::DPadRight) != 0
        || zsdk_input_is_pressed(input, 0, ZsdkButton::Action1) != 0
    {
        adjust_menu_item(st, 1);
    }
    if zsdk_input_is_pressed(input, 0, ZsdkButton::DPadLeft) != 0 {
        adjust_menu_item(st, -1);
    }
    if zsdk_input_is_pressed(input, 0, ZsdkButton::Back) != 0 {
        st.menu_open = false;
    }
}

fn render_hud(st: &mut CubeState, w: i32, h: i32, total_cubes: u16, drawn_cubes: u16) {
    let batch = match st.batch.as_mut() {
        Some(b) => b,
        None => return,
    };
    batch.begin_virtual(640.0, 480.0, w, h);
    batch.draw_rect(10.0, 10.0, 290.0, 36.0, [10, 14, 22, 180]);
    batch.flush();

    let mut line1 = StrBuf::<48>::new();
    line1.push_bytes(b"CUBES: ");
    line1.push_u32(total_cubes as u32);
    line1.push_bytes(b"  DRAWN: ");
    line1.push_u32(drawn_cubes as u32);

    let mut line2 = StrBuf::<48>::new();
    line2.push_bytes(b"FRUSTUM:");
    line2.push_bytes(if st.settings.frustum_cull != 0 { b"ON " } else { b"OFF " });
    line2.push_bytes(b"FACE:");
    line2.push_bytes(if st.settings.face_cull != 0 { b"ON" } else { b"OFF" });

    renderer::draw_text_2d(18.0, 16.0, 1.0, line1.as_cstr(), [235, 245, 255, 255]);
    renderer::draw_text_2d(18.0, 30.0, 1.0, line2.as_cstr(), [160, 215, 255, 255]);
    batch.end();
}

fn render_menu(st: &mut CubeState, w: i32, h: i32) {
    let batch = match st.batch.as_mut() {
        Some(b) => b,
        None => return,
    };
    batch.begin_virtual(640.0, 480.0, w, h);

    let menu_w = 360.0f32;
    let menu_h = 224.0f32;
    let mx = (640.0 - menu_w) * 0.5;
    let my = (480.0 - menu_h) * 0.5;

    batch.draw_rect(mx, my, menu_w, menu_h, [12, 16, 26, 230]);
    batch.draw_rect(mx, my, menu_w, 24.0, [35, 75, 130, 240]);
    let sel_y = my + 34.0 + (st.menu_cursor as f32) * 18.0;
    batch.draw_rect(mx + 8.0, sel_y - 3.0, menu_w - 16.0, 15.0, [55, 110, 180, 200]);
    batch.flush();

    renderer::draw_text_2d(mx + 12.0, my + 8.0, 1.0, c"ZSDK SETTINGS (START)", [255, 255, 255, 255]);

    let mut cube_buf = StrBuf::<40>::new();
    cube_buf.push_bytes(b"Cube Count  : ");
    cube_buf.push_u32(cube_count_at(st.settings.cube_count_index) as u32);

    let fps_label = if (st.settings.fps_mode & 1) != 0 { c"FPS Cap     : 30 FPS" } else { c"FPS Cap     : 60 FPS" };
    let spin_label = match st.settings.spin_mode & 3 {
        0 => c"Spin Speed  : Paused",
        1 => c"Spin Speed  : Slow",
        2 => c"Spin Speed  : Normal",
        _ => c"Spin Speed  : Fast",
    };

    let labels: [&CStr; 10] = [
        cube_buf.as_cstr(),
        if st.settings.frustum_cull != 0 { c"Frustum Cull: ON" } else { c"Frustum Cull: OFF" },
        if st.settings.face_cull != 0 { c"Face Cull   : ON" } else { c"Face Cull   : OFF" },
        if st.settings.lighting != 0 { c"Lighting    : ON" } else { c"Lighting    : OFF" },
        fps_label,
        if st.settings.vsync != 0 { c"VSync       : ON" } else { c"VSync       : OFF" },
        spin_label,
        if DebugOverlay::is_visible() { c"Debug Panel : SHOW" } else { c"Debug Panel : HIDE" },
        c"Save Config (udata)",
        c"Exit App",
    ];

    for (i, label) in labels.iter().enumerate() {
        let ry = my + 34.0 + (i as f32) * 18.0;
        let col = if i as u8 == st.menu_cursor { [255, 240, 120, 255] } else { [210, 220, 230, 255] };
        renderer::draw_text_2d(mx + 16.0, ry, 1.0, label, col);
    }

    batch.end();
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
    let gl = match GLContext::new(platform_ctx, Some(&cfg)) {
        Ok(ctx) => ctx,
        Err(_) => {
            log::error(c"CUBE_RUST", c"GLContext init failed");
            return -1;
        }
    };
    let batch = match SpriteBatch::new(64) {
        Ok(b) => b,
        Err(_) => return -1,
    };
    let frustum = match Frustum::new() {
        Ok(f) => f,
        Err(_) => return -1,
    };

    renderer::configure_light(
        0,
        [6.0, 8.0, 10.0],
        [64, 64, 80, 255],
        [242, 242, 230, 255],
        [204, 204, 204, 255],
    );

    let st = &mut *STATE.0.get();
    st.gl = Some(gl);
    st.batch = Some(batch);
    st.frustum = Some(frustum);
    st.save_mgr = SaveGameManager::new(platform_ctx, false).ok();

    let mut loaded_ok = false;
    if let Some(sg) = st.save_mgr.as_mut() {
        let mut buf = [0u8; size_of::<CubeSettings>()];
        if let Ok(read) = sg.load_slot(0, &mut buf, SAVE_VERSION) {
            if read as usize == size_of::<CubeSettings>() {
                let mut loaded = ptr::read_unaligned(buf.as_ptr() as *const CubeSettings);
                if loaded.cube_count_index > 10 { loaded.cube_count_index = 0; }
                loaded.spin_mode &= 3;
                loaded.frustum_cull = if loaded.frustum_cull != 0 { 1 } else { 0 };
                loaded.face_cull = if loaded.face_cull != 0 { 1 } else { 0 };
                st.settings = loaded;
                apply_settings(st);
                log::info(c"CUBE_RUST", c"Loaded settings from udata.");
                loaded_ok = true;
            }
        }
    }
    if !loaded_ok {
        apply_settings(st);
        log::info(c"CUBE_RUST", c"Spinning Cube ready.");
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_update(delta_ms: u32, input: *const zsdk_input_t) {
    if input.is_null() { return; }
    let dt = (delta_ms as f32) * 0.001;
    let st = &mut *STATE.0.get();

    let menu_btn = zsdk_input_is_pressed(input, 0, ZsdkButton::Start) != 0
        || zsdk_input_is_pressed(input, 0, ZsdkButton::Home) != 0;
    if menu_btn && zsdk_input_is_down(input, 0, ZsdkButton::DPadDown) == 0 {
        st.menu_open = !st.menu_open;
        log::info(c"MENU", if st.menu_open { c"Opened" } else { c"Closed" });
    }

    if st.menu_open {
        update_menu(st, input);
    } else {
        const SPIN_TABLE: [f32; 4] = [0.0, 20.0, 48.0, 110.0];
        let base_spin = SPIN_TABLE[(st.settings.spin_mode & 3) as usize];
        st.angle_y += base_spin * dt;
        st.angle_x += (base_spin * 0.65) * dt;

        let (mut pan_x, mut pan_y) = (0.0f32, 0.0f32);
        zsdk_input_left_stick(input, 0, &mut pan_x, &mut pan_y);
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadLeft)  != 0 { pan_x -= 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadRight) != 0 { pan_x += 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadUp)    != 0 { pan_y += 1.0; }
        if zsdk_input_is_down(input, 0, ZsdkButton::DPadDown)  != 0 { pan_y -= 1.0; }

        st.cam_pan_x = clampf(st.cam_pan_x + pan_x * 16.0 * dt, -40.0, 40.0);
        st.cam_pan_y = clampf(st.cam_pan_y - pan_y * 16.0 * dt, -30.0, 30.0);

        let (mut _rx, mut ry) = (0.0f32, 0.0f32);
        zsdk_input_right_stick(input, 0, &mut _rx, &mut ry);
        st.cam_zoom_offset += ry * 20.0 * dt;
        if zsdk_input_is_down(input, 0, ZsdkButton::ShoulderLeft)  != 0 { st.cam_zoom_offset -= 14.0 * dt; }
        if zsdk_input_is_down(input, 0, ZsdkButton::ShoulderRight) != 0 { st.cam_zoom_offset += 14.0 * dt; }
        st.cam_zoom_offset = clampf(st.cam_zoom_offset, -30.0, 50.0);

        if zsdk_input_is_pressed(input, 0, ZsdkButton::Action1) != 0 {
            st.settings.lighting = if st.settings.lighting == 0 { 1 } else { 0 };
            log::info(c"CUBE_RUST", if st.settings.lighting != 0 { c"Lighting: ON" } else { c"Lighting: OFF" });
        }
        if zsdk_input_is_pressed(input, 0, ZsdkButton::Action2) != 0 {
            st.settings.fps_mode = if st.settings.fps_mode == 1 { 0 } else { 1 };
            apply_fps(st);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_render(out_width: *mut i32, out_height: *mut i32) {
    let st = &mut *STATE.0.get();
    let (w, h) = match st.gl.as_ref() {
        Some(g) => (g.width(), g.height()),
        None => return,
    };

    renderer::set_viewport(0, 0, w, h);
    renderer::clear(16, 20, 32, 255, true, false);

    let aspect = if h > 0 { (w as f32) / (h as f32) } else { 1.333 };
    let fov_y = 55.0f32;
    let near_z = 0.5f32;
    let far_z = 150.0f32;

    let (cols, rows, base_dist) = get_grid_dims(st.settings.cube_count_index);
    if cols == 0 || rows == 0 { return; }
    let cam_dist = clampf(base_dist + st.cam_zoom_offset, 2.5, 90.0);

    let eye = [st.cam_pan_x, st.cam_pan_y, cam_dist];
    let target = [st.cam_pan_x, st.cam_pan_y, 0.0];
    let up = [0.0, 1.0, 0.0];

    renderer::set_perspective(fov_y, aspect, near_z, far_z);
    renderer::reset_modelview();
    renderer::look_at(eye, target, up);

    if st.settings.frustum_cull != 0 {
        if let Some(f) = st.frustum.as_mut() {
            f.extract_camera(eye, target, up, fov_y, aspect, near_z, far_z);
        }
    }

    renderer::set_depth_test(true);
    renderer::set_cull_face(st.settings.face_cull != 0);
    renderer::set_lighting(st.settings.lighting != 0);

    renderer::bind_raw_vertex_arrays(
        ptr::addr_of!(CUBE_VERTICES[0].x) as *const c_void,
        3,
        ptr::addr_of!(CUBE_VERTICES[0].nx) as *const c_void,
        ptr::addr_of!(CUBE_VERTICES[0].r) as *const c_void,
        ptr::null(),
        size_of::<CubeVertex>() as i32,
    );

    let total_cubes = cube_count_at(st.settings.cube_count_index);
    let per_layer = cols * rows;
    if per_layer == 0 { return; }
    let layers = (total_cubes + per_layer - 1) / per_layer;
    let spacing = 3.0f32;
    let half_w = (cols - 1) as f32 * 0.5 * spacing;
    let half_h = (rows - 1) as f32 * 0.5 * spacing;
    let half_d = (layers - 1) as f32 * 0.5 * spacing;
    let cube_radius = 1.75f32;

    let mut drawn = 0u16;
    for i in 0..total_cubes {
        let l = i / per_layer;
        let r = i % per_layer;
        let cx = r % cols;
        let cy = r / cols;

        let px = cx as f32 * spacing - half_w;
        let py = cy as f32 * spacing - half_h;
        let pz = -(l as f32) * spacing + half_d;

        if st.settings.frustum_cull != 0 {
            if let Some(f) = st.frustum.as_ref() {
                if !f.test_sphere([px, py, pz], cube_radius) {
                    continue;
                }
            }
        }

        renderer::push_matrix();
        renderer::translate(px, py, pz);
        renderer::rotate(st.angle_x + ((i & 7) as f32) * 12.0, 1.0, 0.0, 0.0);
        renderer::rotate(st.angle_y + (((i >> 3) & 7) as f32) * 12.0, 0.0, 1.0, 0.0);
        renderer::draw_elements_u16(PrimitiveType::Triangles, &CUBE_INDICES);
        renderer::pop_matrix();
        DebugOverlay::record_draw_call(24);
        drawn += 1;
    }

    renderer::unbind_vertex_attributes();
    renderer::set_lighting(false);

    if total_cubes == 1 {
        renderer::draw_blob_shadow(0.0, -1.6, 0.0, 1.45, [0, 0, 0, 120]);
    }

    renderer::set_depth_test(false);

    render_hud(st, w, h, total_cubes, drawn);
    if st.menu_open {
        render_menu(st, w, h);
    }

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
    let st = &mut *STATE.0.get();
    st.save_mgr = None;
    st.frustum = None;
    st.batch = None;
    st.gl = None;
}
