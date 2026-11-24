/**
 * @file zeebo_audio.cpp
 * @brief Qualcomm BREW / Zeebo IMedia & ISoundPlayer implementation.
 */

#include "zsdk/audio/audio.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEMedia.h"
#include "AEEMediaUtil.h"
#include "AEESoundPlayer.h"

namespace zsdk {
namespace audio {

namespace {

void MediaNotifyCallback(void* userData, AEEMediaCmdNotify* pCmdNotify) {
    if (!userData || !pCmdNotify) {
        return;
    }
    bool* playingFlag = static_cast<bool*>(userData);
    if (pCmdNotify->nCmd == MM_CMD_PLAY) {
        if (pCmdNotify->nStatus == MM_STATUS_DONE ||
            pCmdNotify->nStatus == MM_STATUS_ABORT ||
            pCmdNotify->nStatus == MM_STATUS_MEDIA_SPEC) {
            if (pCmdNotify->nStatus != MM_STATUS_MEDIA_SPEC) {
                *playingFlag = false;
            }
        }
    }
}

} // namespace

struct MediaStream::Impl {
    IMedia* media;
    bool    playing;
    bool    looping;
};

MediaStream::MediaStream()
    : m_impl(nullptr) {
}

MediaStream::~MediaStream() {
    Unload();
}

Result MediaStream::LoadFromFile(void* platformContext, const char* filePath) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !filePath) {
        return Result::InvalidArg;
    }
    Unload();

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Audio, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    MEMSET(m_impl, 0, sizeof(Impl));

    AEEMediaData md;
    md.clsData = MMD_FILE_NAME;
    md.pData   = const_cast<char*>(filePath);
    md.dwSize  = 0;

    if (AEEMediaUtil_CreateMedia(shell, &md, &m_impl->media) != SUCCESS || !m_impl->media) {
        ZSDK_LOG_ERROR("AUD", "AEEMediaUtil_CreateMedia failed for %s", filePath);
        Unload();
        return Result::AudioError;
    }

    IMEDIA_RegisterNotify(m_impl->media, MediaNotifyCallback, &m_impl->playing);
    return Result::Ok;
}

Result MediaStream::LoadFromMemory(void* platformContext, const void* data, uint32_t sizeBytes, MediaFormat fmt) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !data || sizeBytes == 0u) {
        return Result::InvalidArg;
    }
    Unload();

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Audio, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    MEMSET(m_impl, 0, sizeof(Impl));

    ZSDK_UNUSED(fmt);
    AEEMediaData md;
    md.clsData = MMD_BUFFER;
    md.pData   = const_cast<void*>(data);
    md.dwSize  = sizeBytes;

    if (AEEMediaUtil_CreateMedia(shell, &md, &m_impl->media) != SUCCESS ||
        !m_impl->media) {
        ZSDK_LOG_ERROR("AUD", "AEEMediaUtil_CreateMedia (memory) failed");
        Unload();
        return Result::AudioError;
    }

    IMEDIA_RegisterNotify(m_impl->media, MediaNotifyCallback, &m_impl->playing);
    return Result::Ok;
}

void MediaStream::Unload() {
    if (!m_impl) {
        return;
    }
    if (m_impl->media) {
        IMEDIA_Stop(m_impl->media);
        IMEDIA_RegisterNotify(m_impl->media, nullptr, nullptr);
        IMEDIA_Release(m_impl->media);
        m_impl->media = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Audio, m_impl);
    m_impl = nullptr;
}

Result MediaStream::Play() {
    if (!m_impl || !m_impl->media) {
        return Result::NotInitialized;
    }
    m_impl->playing = true;
    const int res = IMEDIA_Play(m_impl->media);
    if (res != SUCCESS) {
        m_impl->playing = false;
        return Result::AudioError;
    }
    return Result::Ok;
}

Result MediaStream::Stop() {
    if (!m_impl || !m_impl->media) {
        return Result::NotInitialized;
    }
    m_impl->playing = false;
    return (IMEDIA_Stop(m_impl->media) == SUCCESS) ? Result::Ok : Result::AudioError;
}

Result MediaStream::Pause() {
    if (!m_impl || !m_impl->media) {
        return Result::NotInitialized;
    }
    return (IMEDIA_Pause(m_impl->media) == SUCCESS) ? Result::Ok : Result::AudioError;
}

Result MediaStream::Resume() {
    if (!m_impl || !m_impl->media) {
        return Result::NotInitialized;
    }
    return (IMEDIA_Resume(m_impl->media) == SUCCESS) ? Result::Ok : Result::AudioError;
}

Result MediaStream::SetVolume(uint8_t volumePercent) {
    if (!m_impl || !m_impl->media) {
        return Result::NotInitialized;
    }
    const uint16 vol = (volumePercent > 100u) ? 100u : volumePercent;
    return (IMEDIA_SetVolume(m_impl->media, vol) == SUCCESS) ? Result::Ok : Result::AudioError;
}

void MediaStream::SetLooping(bool loop) {
    if (m_impl) {
        m_impl->looping = loop;
        if (m_impl->media) {
            IMEDIA_SetMediaParm(m_impl->media, MM_PARM_PLAY_REPEAT, loop ? 0 : 1, 0);
        }
    }
}

bool MediaStream::IsPlaying() const {
    return m_impl ? m_impl->playing : false;
}

bool MediaStream::IsLoaded() const {
    return m_impl && (m_impl->media != nullptr);
}

struct SoundEffect::Impl {
    ISoundPlayer* player;
};

SoundEffect::SoundEffect()
    : m_impl(nullptr) {
}

SoundEffect::~SoundEffect() {
    Shutdown();
}

Result SoundEffect::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Audio, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->player = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_SOUNDPLAYER,
                              reinterpret_cast<void**>(&m_impl->player)) != AEE_SUCCESS || !m_impl->player) {
        ZSDK_LOG_ERROR("AUD", "Failed to create AEECLSID_SOUNDPLAYER");
        Shutdown();
        return Result::AudioError;
    }

    return Result::Ok;
}

void SoundEffect::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->player) {
        ISOUNDPLAYER_Stop(m_impl->player);
        ISOUNDPLAYER_Release(m_impl->player);
        m_impl->player = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Audio, m_impl);
    m_impl = nullptr;
}

Result SoundEffect::PlayFile(const char* filePath) {
    if (!m_impl || !m_impl->player || !filePath) {
        return Result::InvalidArg;
    }
    AEESoundPlayerInfo info;
    MEMSET(&info, 0, sizeof(info));
    info.eInput = SDT_FILE;
    info.pData  = const_cast<char*>(filePath);
    ISOUNDPLAYER_Stop(m_impl->player);
    if (ISOUNDPLAYER_SetInfo(m_impl->player, &info) != SUCCESS) {
        return Result::AudioError;
    }
    ISOUNDPLAYER_Play(m_impl->player);
    return Result::Ok;
}

Result SoundEffect::PlayMemory(const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->player || !data || sizeBytes == 0u) {
        return Result::InvalidArg;
    }
    AEESoundPlayerInfo info;
    MEMSET(&info, 0, sizeof(info));
    info.eInput = SDT_BUFFER;
    info.pData  = const_cast<void*>(data);
    info.dwSize = sizeBytes;
    ISOUNDPLAYER_Stop(m_impl->player);
    if (ISOUNDPLAYER_SetInfo(m_impl->player, &info) != SUCCESS) {
        return Result::AudioError;
    }
    ISOUNDPLAYER_Play(m_impl->player);
    return Result::Ok;
}

Result SoundEffect::Stop() {
    if (!m_impl || !m_impl->player) {
        return Result::NotInitialized;
    }
    ISOUNDPLAYER_Stop(m_impl->player);
    return Result::Ok;
}

Result SoundEffect::SetVolume(uint8_t volumePercent) {
    if (!m_impl || !m_impl->player) {
        return Result::NotInitialized;
    }
    const uint16 vol = (volumePercent > 100u) ? 100u : volumePercent;
    ISOUNDPLAYER_SetVolume(m_impl->player, vol);
    return Result::Ok;
}

} // namespace audio
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
