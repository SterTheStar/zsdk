/**
 * @file cube_c.h
 * @brief Pure C99 interface for the minimal ZSDK C spinning cube + debug panel example.
 */

#ifndef ZSDK_EXAMPLE_CUBE_C_H
#define ZSDK_EXAMPLE_CUBE_C_H

#include "zsdk/c/zsdk_c.h"

ZSDK_C_EXTERN_BEGIN

int  cube_c_init(void* platform_ctx);
void cube_c_update(float dt_sec);
void cube_c_render(int32_t* out_width, int32_t* out_height);
void cube_c_swap(void);
void cube_c_shutdown(void);

ZSDK_C_EXTERN_END

#endif /* ZSDK_EXAMPLE_CUBE_C_H */
