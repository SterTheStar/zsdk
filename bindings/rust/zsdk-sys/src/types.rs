/// C ABI mirror of `zsdk::Result` / `zsdk_result_t` (must match 1:1).
#[repr(i32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkResult {
    Ok                = 0,
    ErrFailed         = -1,
    ErrInvalidArg     = -2,
    ErrOutOfMemory    = -3,
    ErrNotInitialized = -4,
    ErrAlreadyInit    = -5,
    ErrNotSupported   = -6,
    ErrNotFound       = -7,
    ErrWouldBlock     = -8,
    ErrTimeout        = -9,
    ErrIo             = -10,
    ErrDeviceDisconn  = -11,
    ErrGraphics       = -12,
    ErrAudio          = -13,
    ErrNetwork        = -14,
}

pub type zsdk_result_t = ZsdkResult;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkLogLevel {
    Info  = 0,
    Warn  = 1,
    Error = 2,
}

pub type zsdk_log_level_t = ZsdkLogLevel;
