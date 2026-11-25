/**
 * @file zsdk_c_app.h
 * @brief Entry-point callbacks implemented by pure-C or Rust ZSDK applications
 *        when built with zsdk_add_c_app / zsdk_add_rust_app (no main.cpp needed).
 */

#ifndef ZSDK_C_APP_H
#define ZSDK_C_APP_H

#include "zsdk/c/zsdk_c_types.h"
#include "zsdk/c/zsdk_c_input.h"

ZSDK_C_EXTERN_BEGIN

int  zsdk_c_app_init(void* platform_ctx);
void zsdk_c_app_update(uint32_t delta_ms, const zsdk_input_t* input);
void zsdk_c_app_render(int32_t* out_width, int32_t* out_height);
void zsdk_c_app_swap(void);
void zsdk_c_app_shutdown(void);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_APP_H */
