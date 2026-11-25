/**
 * @file zsdk_c_input.h
 * @brief C99 ABI for ZSDK InputManager and GamepadState.
 */

#ifndef ZSDK_C_INPUT_H
#define ZSDK_C_INPUT_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_input_t  zsdk_input_t;
typedef struct zsdk_kbm_t    zsdk_kbm_t;
typedef struct zsdk_vkbd_t   zsdk_vkbd_t;

typedef enum zsdk_button_t {
    ZSDK_BTN_DPAD_UP = 0,
    ZSDK_BTN_DPAD_DOWN,
    ZSDK_BTN_DPAD_LEFT,
    ZSDK_BTN_DPAD_RIGHT,
    ZSDK_BTN_ACTION1,
    ZSDK_BTN_ACTION2,
    ZSDK_BTN_ACTION3,
    ZSDK_BTN_ACTION4,
    ZSDK_BTN_SHOULDER_LEFT,
    ZSDK_BTN_SHOULDER_RIGHT,
    ZSDK_BTN_TRIGGER_LEFT,
    ZSDK_BTN_TRIGGER_RIGHT,
    ZSDK_BTN_START,
    ZSDK_BTN_BACK,
    ZSDK_BTN_HOME,
    ZSDK_BTN_LEFT_THUMB,
    ZSDK_BTN_RIGHT_THUMB,
    ZSDK_BTN_COUNT
} zsdk_button_t;

ZSDK_C_API zsdk_input_t* zsdk_input_create(void);
ZSDK_C_API void          zsdk_input_destroy(zsdk_input_t* input);

ZSDK_C_API zsdk_result_t zsdk_input_init(zsdk_input_t* input, void* platform_ctx);
ZSDK_C_API void          zsdk_input_shutdown(zsdk_input_t* input);
ZSDK_C_API void          zsdk_input_update(zsdk_input_t* input, uint32_t delta_ms);
ZSDK_C_API uint8_t       zsdk_input_connected_count(const zsdk_input_t* input);

ZSDK_C_API int           zsdk_input_is_connected(const zsdk_input_t* input, uint8_t player);
ZSDK_C_API int           zsdk_input_is_down(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn);
ZSDK_C_API int           zsdk_input_is_pressed(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn);
ZSDK_C_API int           zsdk_input_is_released(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn);
ZSDK_C_API void          zsdk_input_left_stick(const zsdk_input_t* input, uint8_t player, float* out_x, float* out_y);
ZSDK_C_API void          zsdk_input_right_stick(const zsdk_input_t* input, uint8_t player, float* out_x, float* out_y);
ZSDK_C_API zsdk_result_t zsdk_input_set_rumble(zsdk_input_t* input, uint8_t player, uint16_t left_motor, uint16_t right_motor);

/* KeyboardMouseManager */
ZSDK_C_API zsdk_kbm_t*   zsdk_kbm_create(void);
ZSDK_C_API void          zsdk_kbm_destroy(zsdk_kbm_t* kbm);
ZSDK_C_API zsdk_result_t zsdk_kbm_init(zsdk_kbm_t* kbm, void* platform_ctx);
ZSDK_C_API void          zsdk_kbm_shutdown(zsdk_kbm_t* kbm);
ZSDK_C_API void          zsdk_kbm_update(zsdk_kbm_t* kbm);
ZSDK_C_API int           zsdk_kbm_is_key_down(const zsdk_kbm_t* kbm, uint16_t hid_key);
ZSDK_C_API void          zsdk_kbm_mouse_delta(const zsdk_kbm_t* kbm, float* out_dx, float* out_dy);
ZSDK_C_API uint8_t       zsdk_kbm_mouse_buttons(const zsdk_kbm_t* kbm);

/* VirtualKeyboard */
ZSDK_C_API zsdk_vkbd_t*  zsdk_vkbd_create(void);
ZSDK_C_API void          zsdk_vkbd_destroy(zsdk_vkbd_t* vkbd);
ZSDK_C_API void          zsdk_vkbd_open(zsdk_vkbd_t* vkbd, const char* title, const char* initial_text, uint8_t max_len);
ZSDK_C_API void          zsdk_vkbd_close(zsdk_vkbd_t* vkbd);
ZSDK_C_API void          zsdk_vkbd_update(zsdk_vkbd_t* vkbd, const zsdk_input_t* input, uint8_t player);
ZSDK_C_API void          zsdk_vkbd_render(const zsdk_vkbd_t* vkbd, int32_t viewport_w, int32_t viewport_h);
ZSDK_C_API int           zsdk_vkbd_is_active(const zsdk_vkbd_t* vkbd);
ZSDK_C_API int           zsdk_vkbd_is_submitted(const zsdk_vkbd_t* vkbd);
ZSDK_C_API int           zsdk_vkbd_is_cancelled(const zsdk_vkbd_t* vkbd);
ZSDK_C_API const char*   zsdk_vkbd_get_text(const zsdk_vkbd_t* vkbd);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_INPUT_H */
