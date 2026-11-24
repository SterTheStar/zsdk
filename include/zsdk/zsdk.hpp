/**
 * @file zsdk.hpp
 * @brief Convenience umbrella header for ZSDK (Zeebo / BREW Platform Abstraction Layer).
 */

#ifndef ZSDK_HPP
#define ZSDK_HPP

#include "zsdk/core/config.hpp"
#include "zsdk/core/types.hpp"
#include "zsdk/core/clock.hpp"
#include "zsdk/core/math_fixed.hpp"
#include "zsdk/core/app.hpp"
#include "zsdk/core/signal.hpp"
#include "zsdk/core/thread.hpp"
#include "zsdk/core/system_info.hpp"

#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"
#include "zsdk/debug/debug_overlay.hpp"

#include "zsdk/input/input_types.hpp"
#include "zsdk/input/input.hpp"
#include "zsdk/input/keyboard_mouse.hpp"
#include "zsdk/input/virtual_keyboard.hpp"

#include "zsdk/graphics/gl_context.hpp"
#include "zsdk/graphics/texture.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/graphics/renderer_types.hpp"
#include "zsdk/graphics/vertex_formats.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/sprite_batch.hpp"
#include "zsdk/graphics/frustum.hpp"
#include "zsdk/graphics/display2d.hpp"
#include "zsdk/graphics/image_decoder.hpp"
#include "zsdk/graphics/sprite2d.hpp"

#include "zsdk/fs/filesystem.hpp"
#include "zsdk/fs/archive_unzip.hpp"
#include "zsdk/fs/local_storage.hpp"
#include "zsdk/fs/ram_cache.hpp"
#include "zsdk/fs/database.hpp"
#include "zsdk/fs/savegame.hpp"

#include "zsdk/audio/audio.hpp"
#include "zsdk/audio/audio_engine.hpp"

#include "zsdk/net/socket.hpp"
#include "zsdk/net/http_client.hpp"
#include "zsdk/net/network_mgr.hpp"
#include "zsdk/net/bluetooth.hpp"

#include "zsdk/security/crypto.hpp"
#include "zsdk/security/license.hpp"

#endif /* ZSDK_HPP */
